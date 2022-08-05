#ifndef VMICORE_LINUX_PROTECTIONVALUES_H
#define VMICORE_LINUX_PROTECTIONVALUES_H

namespace Linux
{
    enum class ProtectionValues
    {
        VM_READ = 1,
        VM_WRITE = 2,
        VM_EXEC = 4,
        VM_SHARED = 8,
    };
}

#endif // VMICORE_LINUX_PROTECTIONVALUES_H
