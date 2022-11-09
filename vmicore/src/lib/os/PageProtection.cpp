#include "PageProtection.h"
#include "linux/ProtectionValues.h"
#include "windows/ProtectionValues.h"
#include <stdexcept>

namespace VmiCore
{
    PageProtection::PageProtection(uint32_t value, OperatingSystem os) : raw(value), os(os)
    {
        switch (os)
        {
            case OperatingSystem::WINDOWS:
            {
                switch (static_cast<Windows::ProtectionValues>(value))
                {
                    case Windows::ProtectionValues::MM_READONLY:
                    {
                        protection = {.readable = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_EXECUTE:
                    {
                        protection = {.executable = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_EXECUTE_READ:
                    {
                        protection = {.readable = 1, .executable = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_READWRITE:
                    {
                        protection = {.readable = 1, .writeable = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_WRITECOPY:
                    {
                        protection = {.readable = 1, .writeable = 1, .copyOnWrite = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_EXECUTE_READWRITE:
                    {
                        protection = {.readable = 1, .writeable = 1, .executable = 1};
                        break;
                    }
                    case Windows::ProtectionValues::MM_EXECUTE_WRITECOPY:
                    {
                        protection = {.readable = 1, .writeable = 1, .executable = 1, .copyOnWrite = 1};
                        break;
                    }
                    default:
                    {
                        throw std::runtime_error("Unknown windows protection value.");
                    }
                }
                break;
            }
            case OperatingSystem::LINUX:
            {
                if (value > UINT8_MAX)
                {
                    throw std::runtime_error("Value must not be larger than a byte.");
                }
                protection = {
                    .readable = static_cast<uint8_t>(value & static_cast<uint8_t>(Linux::ProtectionValues::VM_READ)),
                    .writeable = static_cast<uint8_t>(value & static_cast<uint8_t>(Linux::ProtectionValues::VM_WRITE)),
                    .executable = static_cast<uint8_t>(value & static_cast<uint8_t>(Linux::ProtectionValues::VM_EXEC)),
                    .copyOnWrite =
                        static_cast<uint8_t>((value & (static_cast<uint8_t>(Linux::ProtectionValues::VM_SHARED) |
                                                       static_cast<uint8_t>(Linux::ProtectionValues::VM_MAYWRITE))) ==
                                             static_cast<uint8_t>(Linux::ProtectionValues::VM_MAYWRITE))};
                break;
            }
            default:
            {
                throw std::runtime_error("Invalid operating system");
            }
        }
    }

    ProtectionValues PageProtection::get() const
    {
        return protection;
    }

    uint64_t PageProtection::getRaw() const
    {
        return raw;
    }

    std::string PageProtection::toString() const
    {
        std::string representation;
        if (protection.readable)
        {
            representation.append("R");
        }
        if (protection.writeable)
        {
            representation.append("W");
        }
        if (protection.copyOnWrite)
        {
            representation.append("C");
        }
        if (protection.executable)
        {
            representation.append("X");
        }
        if (representation.empty())
        {
            return "N";
        }

        return representation;
    }
}
