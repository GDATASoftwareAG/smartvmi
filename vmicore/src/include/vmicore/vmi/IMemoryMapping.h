#ifndef VMICORE_IMEMORYMAPPING_H
#define VMICORE_IMEMORYMAPPING_H

#include "MappedRegion.h"
#include <stdexcept>

namespace VmiCore
{
    class MemoryMappingError : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    /**
     * Class representing chunks of memory that were able to be mapped within in a given virtual address range.
     */
    class IMemoryMapping
    {
      public:
        virtual ~IMemoryMapping() = default;

        /**
         * Retrieves a set of memory mapping descriptors. See MappedRegion.h for details. Elements are ordered from
         * lowest to highest guest VA.
         *
         * @throws MemoryMappingError Will occur if unmap has already been called.
         */
        [[nodiscard]] virtual std::span<const MappedRegion> getMappedRegions() const = 0;

        /**
         * Will unmap all mappings. This function will also be called as soon as an instance of this class goes out of
         * scope.
         */
        virtual void unmap() = 0;

      protected:
        IMemoryMapping() = default;
    };
}

#endif // VMICORE_IMEMORYMAPPING_H
