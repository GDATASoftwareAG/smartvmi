#ifndef VMICORE_IBREAKPOINT_H
#define VMICORE_IBREAKPOINT_H

#include "../types.h"

namespace VmiCore
{
    class IBreakpoint
    {
      public:
        virtual ~IBreakpoint() = default;

        [[nodiscard]] virtual addr_t getTargetPA() const = 0;

        virtual void remove() = 0;

      protected:
        IBreakpoint() = default;
    };
}

#endif // VMICORE_IBREAKPOINT_H
