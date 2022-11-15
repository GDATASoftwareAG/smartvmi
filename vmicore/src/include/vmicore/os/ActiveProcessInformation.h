#ifndef VMICORE_ACTIVEPROCESSINFORMATION_H
#define VMICORE_ACTIVEPROCESSINFORMATION_H

#include "IMemoryRegionExtractor.h"
#include <cstdint>
#include <memory>
#include <string>
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
        std::unique_ptr<std::string> fullName;
        std::unique_ptr<std::string> processPath;
        std::unique_ptr<IMemoryRegionExtractor> memoryRegionExtractor;
        bool is32BitProcess;
    };
}

#endif // VMICORE_ACTIVEPROCESSINFORMATION_H
