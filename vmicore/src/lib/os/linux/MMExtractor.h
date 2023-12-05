#ifndef VMICORE_LINUX_MEMORYREGIONEXTRACTOR_H
#define VMICORE_LINUX_MEMORYREGIONEXTRACTOR_H

#include "../../io/ILogging.h"
#include "../../vmi/LibvmiInterface.h"
#include "PathExtractor.h"
#include <vmicore/io/ILogger.h>
#include <vmicore/os/IMemoryRegionExtractor.h>

namespace VmiCore::Linux
{
    class MMExtractor : public IMemoryRegionExtractor
    {
      public:
        MMExtractor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                    const std::shared_ptr<ILogging>& logging,
                    uint64_t mm);

        [[nodiscard]] std::unique_ptr<std::vector<MemoryRegion>> extractAllMemoryRegions() const override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<ILogger> logger;
        PathExtractor pathExtractor;
        uint64_t mm;
    };
}

#endif // VMICORE_LINUX_MEMORYREGIONEXTRACTOR_H
