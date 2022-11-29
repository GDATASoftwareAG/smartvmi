#ifndef VMICORE_BREAKPOINT_H
#define VMICORE_BREAKPOINT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vmicore/types.h>
#include <vmicore/vmi/BpResponse.h>
#include <vmicore/vmi/IBreakpoint.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    class Breakpoint final : public IBreakpoint
    {
      public:
        Breakpoint(uint64_t targetPA,
                   std::function<void(Breakpoint*)> notifyDelete,
                   std::function<BpResponse(IInterruptEvent&)> callback);

        addr_t getTargetPA() const override;

        void remove() override;

        BpResponse callback(IInterruptEvent& event);

      private:
        uint64_t targetPA;
        std::function<void(Breakpoint*)> notifyFunction;
        std::function<BpResponse(IInterruptEvent&)> callbackFunction;
        bool deleted = false;
    };
}

#endif // VMICORE_BREAKPOINT_H
