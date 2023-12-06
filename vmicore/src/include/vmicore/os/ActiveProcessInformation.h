#ifndef VMICORE_ACTIVEPROCESSINFORMATION_H
#define VMICORE_ACTIVEPROCESSINFORMATION_H

#include "IMemoryRegionExtractor.h"
#include <cstdint>
#include <memory>
#include <string>

namespace VmiCore
{
    /// OS-agnostic representation of a process.
    struct ActiveProcessInformation
    {
        /// The base address of the process struct in the kernel.
        uint64_t base;
        /// The pointer to the top level paging structure.
        uint64_t processDtb;
        /// The pointer to the user top level paging structure. Will be the same value as processDtb if either there is
        /// no support for kernel page table isolation or it is turned off.
        uint64_t processUserDtb;
        /// The process ID.
        pid_t pid;
        /// The Process ID of the parent process. Note that a value of zero could mean that either the parent has got a
        /// PID of zero or there is no parent at all.
        pid_t parentPid;
        /// The name of the process. Possibly truncated to a fixed amount of characters which is usually a restriction
        /// of the process struct memory layout.
        std::string name;
        /// The full name of the process without any length restrictions. Extracted from a location other than the
        /// process struct.
        std::unique_ptr<std::string> fullName;
        /// The file path of the executable this process has been started from, if any.
        std::unique_ptr<std::string> processPath;
        /// An object that provides on-demand extraction of memory region descriptors. Parses kernel structures used for
        /// tracking memory allocations of processes.
        std::unique_ptr<IMemoryRegionExtractor> memoryRegionExtractor;
        /// Indicates whether the process is a 32bit process or a 64bit process.
        bool is32BitProcess;
    };
}

#endif // VMICORE_ACTIVEPROCESSINFORMATION_H
