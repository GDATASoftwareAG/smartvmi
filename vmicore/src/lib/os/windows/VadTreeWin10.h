#ifndef VMICORE_WINDOWS_VADTREEWIN10_H
#define VMICORE_WINDOWS_VADTREEWIN10_H

#include "../../io/ILogging.h"
#include "KernelAccess.h"
#include "Vadt.h"
#include <list>
#include <memory>
#include <vector>
#include <vmicore/io/ILogger.h>
#include <vmicore/os/IMemoryRegionExtractor.h>

namespace VmiCore::Windows
{
    class VadTreeWin10 : public IMemoryRegionExtractor
    {
      public:
        VadTreeWin10(std::shared_ptr<IKernelAccess> kernelAccess,
                     uint64_t eprocessBase,
                     pid_t pid,
                     std::string processName,
                     const std::shared_ptr<ILogging>& logging);

        [[nodiscard]] std::unique_ptr<std::vector<MemoryRegion>> extractAllMemoryRegions() const override;

      private:
        std::shared_ptr<IKernelAccess> kernelAccess;
        uint64_t eprocessBase;
        pid_t pid;
        std::string processName;
        std::unique_ptr<ILogger> logger;
        std::vector<uint32_t> mmProtectToValue;

        [[nodiscard]] std::unique_ptr<Vadt> createVadt(uint64_t vadEntryBaseVA) const;

        [[nodiscard]] std::unique_ptr<std::string> extractFileName(addr_t filePointerObjectAddress) const;
    };
}

#endif // VMICORE_WINDOWS_VADTREEWIN10_H
