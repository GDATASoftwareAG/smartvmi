#ifndef VMICORE_WINDOWS_PROTECTIONVALUES_H
#define VMICORE_WINDOWS_PROTECTIONVALUES_H

namespace VmiCore::Windows
{
    enum class ProtectionValues
    {
        MM_ZERO_ACCESS = 0,
        MM_READONLY = 1,
        MM_EXECUTE = 2,
        MM_EXECUTE_READ = 3,
        MM_READWRITE = 4,
        MM_WRITECOPY = 5,
        MM_EXECUTE_READWRITE = 6,
        MM_EXECUTE_WRITECOPY = 7,
    };
}

#endif // VMICORE_WINDOWS_PROTECTIONVALUES_H
