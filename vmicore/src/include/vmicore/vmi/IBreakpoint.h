#ifndef VMICORE_IBREAKPOINT_H
#define VMICORE_IBREAKPOINT_H

#include "../types.h"
#include "BpResponse.h"
#include "events/IInterruptEvent.h"
#include <functional>
#include <memory>

namespace VmiCore
{
    class IBreakpoint
    {
      public:
        virtual ~IBreakpoint() = default;

        virtual addr_t getTargetPA() const = 0;

        virtual void remove() = 0;

        template <class T>
        static std::function<BpResponse(IInterruptEvent&)>
        createBreakpointCallback(const std::weak_ptr<T>& callbackObject,
                                 BpResponse (T::*callbackFunction)(IInterruptEvent&))
        {
            return [callbackObject = callbackObject, callbackFunction = callbackFunction](IInterruptEvent& event)
            {
                if (auto targetShared = callbackObject.lock())
                {
                    return ((*targetShared).*callbackFunction)(event);
                }

                throw std::runtime_error("Callback target does not exist anymore.");
            };
        }

      protected:
        IBreakpoint() = default;
    };
}

#endif // VMICORE_IBREAKPOINT_H
