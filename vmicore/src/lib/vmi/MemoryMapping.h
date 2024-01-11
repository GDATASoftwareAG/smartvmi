#ifndef VMICORE_MEMORYMAPPING_H
#define VMICORE_MEMORYMAPPING_H

#include "../io/ILogging.h"
#include "LibvmiInterface.h"
#include <vmicore/vmi/IMemoryMapping.h>

namespace VmiCore
{
    class MemoryMapping final : public IMemoryMapping
    {
      public:
        MemoryMapping(const std::shared_ptr<ILogging>& logging,
                      std::shared_ptr<ILibvmiInterface> vmiInterface,
                      mapped_regions_t mappedRegions);

        ~MemoryMapping() override;

        MemoryMapping(const MemoryMapping&) = delete;

        MemoryMapping(const MemoryMapping&&) = delete;

        MemoryMapping& operator=(const MemoryMapping&) = delete;

        MemoryMapping& operator=(const MemoryMapping&&) = delete;

        [[nodiscard]] std::span<const MappedRegion> getMappedRegions() const override;

        void unmap() override;

      private:
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        mapped_regions_t libvmiMappings;
        bool isMapped = true;
    };
}

#endif // VMICORE_MEMORYMAPPING_H
