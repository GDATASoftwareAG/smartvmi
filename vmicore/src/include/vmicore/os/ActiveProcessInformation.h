#ifndef VMICORE_ACTIVEPROCESSINFORMATION_H
#define VMICORE_ACTIVEPROCESSINFORMATION_H

#include "../vmi/IVmiUnicodeStruct.h"
#include "IMemoryRegionExtractor.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace VmiCore
{
    struct ActiveProcessInformation
    {
        uint64_t base;
        uint64_t processCR3;
        pid_t pid;
        pid_t parentPid;
        std::string name;
        std::string_view fullName;
        std::string_view processPath;
        std::variant<std::string, std::unique_ptr<IVmiUnicodeStruct>> processPathData;
        std::unique_ptr<IMemoryRegionExtractor> memoryRegionExtractor;
        bool is32BitProcess;
    };
}

#endif // VMICORE_ACTIVEPROCESSINFORMATION_H
