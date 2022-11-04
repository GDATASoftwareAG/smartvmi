#ifndef VMICORE_MEMORYREGION_H
#define VMICORE_MEMORYREGION_H

#include "../types.h"
#include "IPageProtection.h"
#include <memory>
#include <string>
#include <utility>

namespace VmiCore
{
    struct MemoryRegion
    {
        MemoryRegion(addr_t base,
                     std::size_t size,
                     std::string moduleName,
                     std::unique_ptr<IPageProtection> protection,
                     bool isSharedMemory,
                     bool isBeingDeleted,
                     bool isProcessBaseImage)
            : base(base),
              size(size),
              moduleName(std::move(moduleName)),
              protection(std::move(protection)),
              isSharedMemory(isSharedMemory),
              isBeingDeleted(isBeingDeleted),
              isProcessBaseImage(isProcessBaseImage)
        {
        }

        addr_t base;
        std::size_t size;
        std::string moduleName;
        std::unique_ptr<IPageProtection> protection;
        bool isSharedMemory;
        bool isBeingDeleted;
        bool isProcessBaseImage;
    };
}

#endif // VMICORE_MEMORYREGION_H
