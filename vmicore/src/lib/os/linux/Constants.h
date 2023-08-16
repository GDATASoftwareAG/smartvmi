#ifndef VMICORE_LINUX_CONSTANTS_H
#define VMICORE_LINUX_CONSTANTS_H

namespace VmiCore::Linux
{
    constexpr pid_t SYSTEM_PID = 0;
    constexpr uint16_t USER_DTB_OFFSET = 0x1000;
    constexpr auto PTI_FEATURE_ARRAY_ENTRY_OFFSET = 7 * sizeof(uint32_t);
    constexpr uint64_t PTI_FEATURE_MASK = 1ULL << 11;
}

#endif // VMICORE_LINUX_CONSTANTS_H
