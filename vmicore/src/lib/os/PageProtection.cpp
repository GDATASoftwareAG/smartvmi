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
                protection = {.readable = static_cast<uint8_t>((value & Windows::READ_MASK) != 0),
                              .writeable = static_cast<uint8_t>((value & Windows::WRITE_MASK) != 0),
                              .executable = static_cast<uint8_t>((value & Windows::EXEC_MASK) != 0),
                              .copyOnWrite = static_cast<uint8_t>((value & Windows::WRITE_COPY_MASK) != 0)};
                break;
            }
            case OperatingSystem::LINUX:
            {
                if (value > UINT8_MAX)
                {
                    throw std::runtime_error("Value must not be larger than a byte.");
                }
                protection = {
                    .readable =
                        static_cast<uint8_t>((value & static_cast<uint8_t>(Linux::ProtectionValues::VM_READ)) != 0),
                    .writeable =
                        static_cast<uint8_t>((value & static_cast<uint8_t>(Linux::ProtectionValues::VM_WRITE)) != 0),
                    .executable =
                        static_cast<uint8_t>((value & static_cast<uint8_t>(Linux::ProtectionValues::VM_EXEC)) != 0),
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
