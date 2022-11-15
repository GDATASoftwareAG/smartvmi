#ifndef VMICORE_MEMORYMAPPING_H
#define VMICORE_MEMORYMAPPING_H

#include "../io/ILogging.h"
#include <memory>
#include <vector>
#include <vmicore/types.h>
#include <vmicore/vmi/IMemoryMapping.h>

namespace VmiCore
{
    class MemoryMapping final : public IMemoryMapping
    {
      public:
        MemoryMapping(addr_t guestBaseVA,
                      const std::vector<void*>& accessPointers,
                      const std::shared_ptr<ILogging>& logging);

        ~MemoryMapping() override;

        MemoryMapping(const MemoryMapping&) = delete;

        MemoryMapping(const MemoryMapping&&) = delete;

        MemoryMapping& operator=(const MemoryMapping&) = delete;

        MemoryMapping& operator=(const MemoryMapping&&) = delete;

        std::weak_ptr<std::vector<MappedRegion>> getMappedRegions() override;

        size_t getSizeInGuest() override;

        void unmap() override;

      private:
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<std::vector<MappedRegion>> mappings;
        std::size_t sizeInGuest = 0;
        std::size_t mappingSize = 0;
        bool isMapped = true;
    };
} // VmiCore

#endif // VMICORE_MEMORYMAPPING_H
