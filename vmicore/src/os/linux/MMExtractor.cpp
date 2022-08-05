#include "MMExtractor.h"
#include "Constants.h"
#include "ProtectionValues.h"

namespace Linux
{
    MMExtractor::MMExtractor(const std::shared_ptr<ILibvmiInterface>& vmiInterface,
                             const std::shared_ptr<ILogging>& logging,
                             uint64_t mm)
        : vmiInterface(vmiInterface), logger(NEW_LOGGER(logging)), pathExtractor(vmiInterface, logging), mm(mm)
    {
    }

    std::unique_ptr<std::list<MemoryRegion>> MMExtractor::extractAllMemoryRegions() const
    {
        auto regions = std::make_unique<std::list<MemoryRegion>>();

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

            auto permissions = PageProtection(flags, OperatingSystem::LINUX);

            logger->debug("Memory Region",
                          {logfield::create("start", fmt::format("{:#x}", start)),
                           logfield::create("end", fmt::format("{:#x}", end)),
                           logfield::create("size", size),
                           logfield::create("permissions", permissions.toString()),
                           logfield::create("filename", fileName)});
            regions->emplace_back(start,
                                  size,
                                  fileName,
                                  permissions,
                                  !!(flags & static_cast<uint8_t>(ProtectionValues::VM_SHARED)),
                                  false,
                                  false);
        }

        return regions;
    }
}
