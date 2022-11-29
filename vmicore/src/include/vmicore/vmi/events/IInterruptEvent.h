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

        [[nodiscard]] virtual addr_t getGla() const = 0;

        [[nodiscard]] virtual addr_t getGfn() const = 0;

        [[nodiscard]] virtual addr_t getOffset() const = 0;

      protected:
        IInterruptEvent() = default;
    };
}

#endif // VMICORE_IINTERRUPTEVENT_H
