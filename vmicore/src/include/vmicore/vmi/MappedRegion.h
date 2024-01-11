#ifndef VMICORE_MAPPEDREGION_H
#define VMICORE_MAPPEDREGION_H

#include "../os/PagingDefinitions.h"
#include "../types.h"
#include <cstdint>
#include <span>
#include <stdexcept>

namespace VmiCore
{
    /**
     * Represents a chunk of contiguous memory that has been mapped into the address space of the introspection
     * application.
     */
    struct MappedRegion
    {
        /// A virtual address representing the start of the memory region inside the guest.
        addr_t guestBaseVA;
        /// Size of the memory region in pages. Will always be the finest granularity, even if the guest uses large
        /// pages.
        std::size_t num_pages;
        /// Base address of the mapped memory region inside the introspection application's address space.
        void* mappingBase;

        MappedRegion(addr_t guestBaseVA, std::size_t num_pages, void* mappingBase)
            : guestBaseVA(guestBaseVA), num_pages(num_pages), mappingBase(mappingBase)
        {
        }

        MappedRegion(addr_t guestBaseVA, std::span<uint8_t> mapping)
            : guestBaseVA(guestBaseVA),
              num_pages(mapping.size() / PagingDefinitions::pageSizeInBytes),
              mappingBase(static_cast<void*>(mapping.data()))
        {
            if (mapping.size() % PagingDefinitions::pageSizeInBytes != 0)
            {
                throw std::invalid_argument("Mapping has to be page aligned");
            }
        }

        /**
         * Convenience method for safe access to the mapped memory.
         */
        [[nodiscard]] std::span<const uint8_t> asSpan() const
        {
            return {static_cast<uint8_t*>(mappingBase), num_pages * PagingDefinitions::pageSizeInBytes};
        }

        bool operator==(const MappedRegion&) const = default;
    };
}

#endif // VMICORE_MAPPEDREGION_H
