#ifndef VMICORE_PAGINGDEFINITIONS_H
#define VMICORE_PAGINGDEFINITIONS_H

#include <cstdint>

namespace VmiCore::PagingDefinitions
{
    constexpr uint64_t numberOfPageIndexBits = 12;
    constexpr uint64_t pageSizeInBytes = 4096;
    constexpr uint64_t pageOffsetMask = pageSizeInBytes - 1;
    constexpr uint64_t stripPageOffsetMask = ~pageOffsetMask;
    constexpr uint64_t kernelspaceLowerBoundary = 0xFFFF800000000000;
}

#endif // VMICORE_PAGINGDEFINITIONS_H
