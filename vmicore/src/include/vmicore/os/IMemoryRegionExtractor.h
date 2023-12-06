#ifndef VMICORE_IMEMORYREGIONEXTRACTOR_H
#define VMICORE_IMEMORYREGIONEXTRACTOR_H

#include "MemoryRegion.h"
#include <memory>
#include <vector>

namespace VmiCore
{
    class IMemoryRegionExtractor
    {
      public:
        virtual ~IMemoryRegionExtractor() = default;

        /**
         * Provides a representation of all memory regions for a specific process. Does not guarantee any ordering.
         */
        [[nodiscard]] virtual std::unique_ptr<std::vector<MemoryRegion>> extractAllMemoryRegions() const = 0;

      protected:
        IMemoryRegionExtractor() = default;
    };
}

#endif // VMICORE_IMEMORYREGIONEXTRACTOR_H
