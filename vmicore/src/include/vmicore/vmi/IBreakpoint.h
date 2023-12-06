#ifndef VMICORE_IBREAKPOINT_H
#define VMICORE_IBREAKPOINT_H

#include "../types.h"

namespace VmiCore
{
    /**
     * An abstract handle of a breakpoint that has been inserted into the guest.
     */
    class IBreakpoint
    {
      public:
        virtual ~IBreakpoint() = default;

        /**
         * Retrieve the target gpa the breakpoint has been placed on.
         */
        [[nodiscard]] virtual addr_t getTargetPA() const = 0;

        /**
         * Remove the breakpoint. This will only guarantee that the creator of this breakpoint stops receiving
         * callbacks. The actual physical breakpoint might still exist in the guest depending on whether there are
         * multiple breakpoint objects subscribed to the same physical breakpoint or just a single one.
         */
        virtual void remove() = 0;

      protected:
        IBreakpoint() = default;
    };
}

#endif // VMICORE_IBREAKPOINT_H
