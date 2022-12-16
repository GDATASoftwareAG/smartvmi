#ifndef VMICORE_WINDOWS_PROTECTIONVALUES_H
#define VMICORE_WINDOWS_PROTECTIONVALUES_H

namespace VmiCore::Windows
{
    // these are from WinNT.h
    enum class ProtectionValues
    {
        PAGE_NOACCESS = 0x01,
        PAGE_READONLY = 0x02,
        PAGE_READWRITE = 0x04,
        PAGE_WRITECOPY = 0x08,
        PAGE_EXECUTE = 0x10,
        PAGE_EXECUTE_READ = 0x20,
        PAGE_EXECUTE_READWRITE = 0x40,
        PAGE_EXECUTE_WRITECOPY = 0x80,
        PAGE_GUARD = 0x100,
        PAGE_NOCACHE = 0x200,
        PAGE_WRITECOMBINE = 0x400,
        PAGE_TARGETS_INVALID = 0x40000000,
    };

    constexpr uint32_t READ_MASK = static_cast<uint32_t>(ProtectionValues::PAGE_READONLY) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_WRITECOPY) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_READ) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_READWRITE) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY);

    constexpr uint32_t WRITE_MASK = static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE) |
                                    static_cast<uint32_t>(ProtectionValues::PAGE_WRITECOPY) |
                                    static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_READWRITE) |
                                    static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY);

    constexpr uint32_t EXEC_MASK = static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_READ) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_READWRITE) |
                                   static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY);

    constexpr uint32_t WRITE_COPY_MASK = static_cast<uint32_t>(ProtectionValues::PAGE_WRITECOPY) |
                                         static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY);
}

#endif // VMICORE_WINDOWS_PROTECTIONVALUES_H
