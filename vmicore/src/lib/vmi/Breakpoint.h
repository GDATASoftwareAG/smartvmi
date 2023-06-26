#ifndef VMICORE_BREAKPOINT_H
#define VMICORE_BREAKPOINT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vmicore/types.h>
#include <vmicore/vmi/BpResponse.h>
#include <vmicore/vmi/IBreakpoint.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    enum class BPStateResponse
    {
        Disable,
        Enable
    };

    class Breakpoint final : public IBreakpoint
    {
      public:
        Breakpoint(uint64_t targetPA,
                   std::function<void(Breakpoint*)> notifyDelete,
                   std::function<BpResponse(IInterruptEvent&)> callback,
                   uint64_t dtb,
                   bool global);

        addr_t getTargetPA() const override;

        void remove() override;

        BpResponse callback(IInterruptEvent& event);

        BPStateResponse getNewBreakpointState(uint64_t dtb) const;

      private:
        uint64_t targetPA;
        std::unordered_set<uint64_t> hookedProcesses;
        std::function<void(Breakpoint*)> notifyFunction;
        std::function<BpResponse(IInterruptEvent&)> callbackFunction;
        bool deleted = false;
        uint64_t dtb;
        bool global = false;
    };
}

#endif // VMICORE_BREAKPOINT_H
