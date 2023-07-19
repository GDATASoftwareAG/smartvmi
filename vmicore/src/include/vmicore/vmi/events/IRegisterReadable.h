#ifndef VMICORE_IREGISTERREADABLE_H
#define VMICORE_IREGISTERREADABLE_H

#include <cstdint>

namespace VmiCore
{
    class IRegisterReadable
    {
      public:
        virtual ~IRegisterReadable() = default;

        [[nodiscard]] virtual uint64_t getRax() const = 0;

        [[nodiscard]] virtual uint64_t getRbx() const = 0;

        [[nodiscard]] virtual uint64_t getRcx() const = 0;

        [[nodiscard]] virtual uint64_t getRdx() const = 0;

        [[nodiscard]] virtual uint64_t getRdi() const = 0;

        [[nodiscard]] virtual uint64_t getR8() const = 0;

        [[nodiscard]] virtual uint64_t getR9() const = 0;

        [[nodiscard]] virtual uint64_t getRip() const = 0;

        [[nodiscard]] virtual uint64_t getRsp() const = 0;

        [[nodiscard]] virtual uint64_t getCr3() const = 0;

        [[nodiscard]] virtual uint64_t getGs() const = 0;

      protected:
        IRegisterReadable() = default;
    };
}

#endif // VMICORE_IREGISTERREADABLE_H
