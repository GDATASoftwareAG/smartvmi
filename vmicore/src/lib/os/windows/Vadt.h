#ifndef VMICORE_WINDOWS_VADT_H
#define VMICORE_WINDOWS_VADT_H

#include "ProtectionValues.h"
#include <string>

namespace VmiCore::Windows
{
    class Vadt
    {
      public:
        uint64_t vadEntryBaseVA;
        uint64_t startingVPN;
        uint64_t endingVPN;
        VmiUnicodeStruct fileName;
        uint8_t protection;
        bool isFileBacked;
        bool isSharedMemory;
        bool isBeingDeleted;
        bool isProcessBaseImage;

        bool operator==(const Vadt& rhs) const
        {
            return (vadEntryBaseVA != rhs.vadEntryBaseVA)           ? false
                   : (startingVPN != rhs.startingVPN)               ? false
                   : (endingVPN != rhs.endingVPN)                   ? false
                   : (fileName != rhs.fileName)                     ? false
                   : (protection != rhs.protection)                 ? false
                   : (isFileBacked != rhs.isFileBacked)             ? false
                   : (isSharedMemory != rhs.isSharedMemory)         ? false
                   : (isProcessBaseImage != rhs.isProcessBaseImage) ? false
                                                                    : true;
        }

        bool operator!=(const Vadt& rhs) const
        {
            return !operator==(rhs);
        }
    };
}

#endif // VMICORE_WINDOWS_VADT_H
