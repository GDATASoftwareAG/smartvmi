#ifndef VMICORE_IINTERRUPTEVENT_H
#define VMICORE_IINTERRUPTEVENT_H

#include "../../types.h"
#include "IRegisterReadable.h"

namespace VmiCore
{
    class IInterruptEvent : public IRegisterReadable
    {
      public:
        ~IInterruptEvent() override = default;

        virtual addr_t getGla() = 0;

        virtual addr_t getGfn() = 0;

        virtual addr_t getOffset() = 0;

      protected:
        IInterruptEvent() = default;
    };
}

#endif // VMICORE_IINTERRUPTEVENT_H
