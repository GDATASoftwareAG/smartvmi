#ifndef VMICORE_IREGISTERREADABLE_H
#define VMICORE_IREGISTERREADABLE_H

#include <cstdint>

namespace VmiCore
{
    class IRegisterReadable
    {
      public:
        virtual ~IRegisterReadable() = default;

        virtual uint64_t getRax() = 0;

        virtual uint64_t getRbx() = 0;

        virtual uint64_t getRcx() = 0;

        virtual uint64_t getRdx() = 0;

        virtual uint64_t getRdi() = 0;

        virtual uint64_t getR8() = 0;

        virtual uint64_t getR9() = 0;

        virtual uint64_t getRip() = 0;

        virtual uint64_t getCr3() = 0;

      protected:
        IRegisterReadable() = default;
    };
}

#endif // VMICORE_IREGISTERREADABLE_H
