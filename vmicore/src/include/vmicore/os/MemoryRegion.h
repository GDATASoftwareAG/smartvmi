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

        /// The start address of the memory region.
        addr_t base;
        /// The size of the memory region in bytes.
        std::size_t size;
        /**
         * If the memory region is file-backed, contains the path of the file on disk. If an exception occurred during
         * extraction, will contain the special string "unknownFilename". Will be an empty string otherwise.
         *
         * @note For linux guests, this string might be empty or partially extracted if an error is encountered instead
         * of being set to "unknownFilename". This behavior might be adjusted in the future for increased consistency.
         */
        std::string moduleName;
        /// An object representing the protection values for the memory region. See
        /// <a href=./IPageProtection.h>IPageProtection.h</a> for details.
        std::unique_ptr<IPageProtection> protection;
        /// Indicates whether this memory region can be shared between processes.
        bool isSharedMemory;
        /// Indicates whether this memory is marked for deletion. Always false for linux guests.
        bool isBeingDeleted;
        /**
         * Indicates whether this memory region represents the executable the process has been initiated from.
         *
         * @note Currently always false for linux guests.
         */
        bool isProcessBaseImage;
    };
}

#endif // VMICORE_MEMORYREGION_H
