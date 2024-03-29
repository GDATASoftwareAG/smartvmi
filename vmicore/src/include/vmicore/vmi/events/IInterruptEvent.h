#ifndef VMICORE_IINTERRUPTEVENT_H
#define VMICORE_IINTERRUPTEVENT_H

#include "../../types.h"
#include "IRegisterReadable.h"

namespace VmiCore
{
    /**
     * A readonly representation of an interrupt event that has been generated by a software breakpoint. Will not be
     * valid outside the scope of a user callback.
     */
    class IInterruptEvent : public IRegisterReadable
    {
      public:
        ~IInterruptEvent() override = default;

        /**
         * Retrieve the virtual address of the event.
         */
        [[nodiscard]] virtual addr_t getGla() const = 0;

        /**
         * Retrieve the guest frame number of the event.
         */
        [[nodiscard]] virtual addr_t getGfn() const = 0;

        /**
         * Retrieve the page offset of the event.
         */
        [[nodiscard]] virtual addr_t getOffset() const = 0;

      protected:
        IInterruptEvent() = default;
    };
}

#endif // VMICORE_IINTERRUPTEVENT_H
