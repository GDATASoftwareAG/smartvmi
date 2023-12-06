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

        /**
         * Get an OS-agnostic representation of protection values.
         */
        [[nodiscard]] virtual ProtectionValues get() const = 0;

        /**
         * Get protection values in exactly the same representation as they have been extracted from memory. Not
         * OS-agnostic, but may provide more information.
         */
        [[nodiscard]] virtual uint64_t getRaw() const = 0;

        /**
         * Returns a string representation of the protection values.
         */
        [[nodiscard]] virtual std::string toString() const = 0;

      protected:
        IPageProtection() = default;
    };
}

#endif // VMICORE_IPAGEPROTECTION_H
