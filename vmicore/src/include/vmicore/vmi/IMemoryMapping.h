#ifndef VMICORE_IMEMORYMAPPING_H
#define VMICORE_IMEMORYMAPPING_H

#include "MappedRegion.h"
#include <memory>
#include <vector>

namespace VmiCore
{
    class IMemoryMapping
    {
      public:
        virtual ~IMemoryMapping() = default;

        IMemoryMapping(const IMemoryMapping&) = delete;

        IMemoryMapping(const IMemoryMapping&&) = delete;

        IMemoryMapping& operator=(const IMemoryMapping&) = delete;

        IMemoryMapping& operator=(const IMemoryMapping&&) = delete;

        virtual std::weak_ptr<std::vector<MappedRegion>> getMappedRegions() = 0;

        virtual std::size_t getSizeInGuest() = 0;

        virtual void unmap() = 0;

      protected:
        IMemoryMapping() = default;
    };
}

#endif // VMICORE_IMEMORYMAPPING_H
