#include "Breakpoint.h"
#include "VmiException.h"
#include <fmt/core.h>
#include <utility>
#include <vmicore/os/PagingDefinitions.h>

namespace VmiCore
{
    Breakpoint::Breakpoint(uint64_t targetPA,
                           std::function<void(Breakpoint*)> notifyDelete,
                           std::function<BpResponse(IInterruptEvent&)> callback,
                           uint64_t processDtb,
                           bool global)
        : targetPA(targetPA),
          notifyFunction(std::move(notifyDelete)),
          callbackFunction(std::move(callback)),
          dtb(processDtb),
          global(global)
    {
    }

    addr_t Breakpoint::getTargetPA() const
    {
        return targetPA;
    }

    void Breakpoint::remove()
    {
        if (!deleted)
        {
            notifyFunction(this);
            deleted = true;
        }
    }

    BpResponse Breakpoint::callback(IInterruptEvent& event)
    {
        if (!global && event.getCr3() != dtb)
        {
            return BpResponse::Continue;
        }
        try
        {
            return callbackFunction(event);
        }
        catch (const std::runtime_error& e)
        {
            throw std::runtime_error(
                fmt::format("{}: {} Target physical address = {:#x}", __func__, e.what(), targetPA));
        }
    }
    BPStateResponse Breakpoint::getNewBreakpointState(uint64_t newDtb) const
    {
        using enum VmiCore::BPStateResponse;

        if (global) // Breakpoint always present e.g. in specific kernel functions
        {
            return Enable;
        }

        if (dtb == newDtb)
        {
            return Enable;
        }
        // Process is not hooked
        return Disable;
    }
}
