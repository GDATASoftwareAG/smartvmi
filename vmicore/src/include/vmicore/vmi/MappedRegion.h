#ifndef VMICORE_MAPPEDREGION_H
#define VMICORE_MAPPEDREGION_H

#include "../types.h"
#include <cstddef>
#include <span>

namespace VmiCore
{
    struct MappedRegion
    {
        addr_t guestBaseVA;
        std::span<uint8_t> mapping;

        MappedRegion(addr_t guestBaseVA, std::span<uint8_t> region) : guestBaseVA(guestBaseVA), mapping(region){};

        bool operator==(const MappedRegion& rhs) const
        {
            return guestBaseVA == rhs.guestBaseVA && mapping.data() == rhs.mapping.data() &&
                   mapping.size() == rhs.mapping.size();
        }

        bool operator!=(const MappedRegion& rhs) const
        {
            return !(rhs == *this);
        }
    };
}

#endif // VMICORE_MAPPEDREGION_H
