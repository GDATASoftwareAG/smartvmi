#ifndef VMICORE_IPAGEPROTECTION_H
#define VMICORE_IPAGEPROTECTION_H

#include <cstdint>
#include <string>

namespace VmiCore
{
    struct ProtectionValues
    {
        uint8_t readable : 1 = 0;
        uint8_t writeable : 1 = 0;
        uint8_t executable : 1 = 0;
        uint8_t copyOnWrite : 1 = 0;
    };

    class IPageProtection
    {
      public:
        virtual ~IPageProtection() = default;

        [[nodiscard]] virtual ProtectionValues get() const = 0;

        [[nodiscard]] virtual uint64_t getRaw() const = 0;

        [[nodiscard]] virtual std::string toString() const = 0;

      protected:
        IPageProtection() = default;
    };
}

#endif // VMICORE_IPAGEPROTECTION_H
