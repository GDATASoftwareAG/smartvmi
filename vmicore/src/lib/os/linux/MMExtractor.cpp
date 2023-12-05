#include "MMExtractor.h"
#include "../PageProtection.h"
#include "Constants.h"
#include "ProtectionValues.h"
#include <vmicore/filename.h>

namespace VmiCore::Linux
{
    MMExtractor::MMExtractor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                             const std::shared_ptr<ILogging>& logging,
                             uint64_t mm)
        : vmiInterface(std::move(vmiInterface)),
          logger(logging->newNamedLogger(FILENAME_STEM)),
          pathExtractor(this->vmiInterface, logging),
          mm(mm)
    {
    }

    std::unique_ptr<std::vector<MemoryRegion>> MMExtractor::extractAllMemoryRegions() const
    {
        auto regions = std::make_unique<std::vector<MemoryRegion>>();

        for (auto area = vmiInterface->read64VA(mm, vmiInterface->convertPidToDtb(SYSTEM_PID)); area != 0;
             area = vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_next"),
                                           vmiInterface->convertPidToDtb(SYSTEM_PID)))
        {
            const auto start =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_start"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto end =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_end"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto size = end - start + 1;
            const auto flags =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_flags"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto file =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_file"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            std::string fileName{};
            if (file != 0)
            {
                fileName = pathExtractor.extractDPath(file + vmiInterface->getKernelStructOffset("file", "f_path"));
            }

            auto permissions = std::make_unique<PageProtection>(flags, OperatingSystem::LINUX);

            logger->debug("Memory Region",
                          {{"start", fmt::format("{:#x}", start)},
                           {"end", fmt::format("{:#x}", end)},
                           {"size", size},
                           {"permissions", permissions->toString()},
                           {"filename", fileName}});
            regions->emplace_back(start,
                                  size,
                                  fileName,
                                  std::move(permissions),
                                  !!(flags & static_cast<uint8_t>(ProtectionValues::VM_SHARED)),
                                  false,
                                  false);
        }

        return regions;
    }
}
