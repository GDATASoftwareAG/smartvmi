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

        VM_MAYREAD = 0x10,
        VM_MAYWRITE = 0x20,
        VM_MAYEXEC = 0x40,
        VM_MAYSHARE = 0x80
    };
}

#endif // VMICORE_LINUX_PROTECTIONVALUES_H
