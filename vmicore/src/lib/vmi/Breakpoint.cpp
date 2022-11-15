#include "Breakpoint.h"
#include "VmiException.h"
#include <fmt/core.h>
#include <utility>
#include <vmicore/os/PagingDefinitions.h>

namespace VmiCore
{
    Breakpoint::Breakpoint(uint64_t targetPA,
                           std::function<void(Breakpoint*)> notifyDelete,
                           std::function<BpResponse(IInterruptEvent&)> callback)
        : targetPA(targetPA), notifyFunction(std::move(notifyDelete)), callbackFunction(std::move(callback))
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
}
