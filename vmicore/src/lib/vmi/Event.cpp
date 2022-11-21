#include "Event.h"
#include <fmt/core.h>
#include <stdexcept>

namespace VmiCore
{
    Event::Event(vmi_event_t* libvmiEvent) : libvmiEvent(libvmiEvent) {}

    uint64_t Event::getRax()
    {
        return libvmiEvent->x86_regs->rax;
    }

    uint64_t Event::getRbx()
    {
        return libvmiEvent->x86_regs->rbx;
    }

    uint64_t Event::getRcx()
    {
        return libvmiEvent->x86_regs->rcx;
    }

    uint64_t Event::getRdx()
    {
        return libvmiEvent->x86_regs->rdx;
    }

    uint64_t Event::getRdi()
    {
        return libvmiEvent->x86_regs->rdi;
    }

    uint64_t Event::getR8()
    {
        return libvmiEvent->x86_regs->r8;
    }

    uint64_t Event::getR9()
    {
        return libvmiEvent->x86_regs->r9;
    }

    uint64_t Event::getRip()
    {
        return libvmiEvent->x86_regs->rip;
    }

    uint64_t Event::getCr3()
    {
        return libvmiEvent->x86_regs->cr3;
    }

    addr_t Event::getGla()
    {
        switch (libvmiEvent->type)
        {
            case VMI_EVENT_INTERRUPT:
            {
                if (libvmiEvent->interrupt_event.intr != INT3)
                {
                    throw std::runtime_error("Invalid union access");
                }
                return libvmiEvent->interrupt_event.gla;
            }
            default:
            {
                throw std::runtime_error(
                    fmt::format("{} not implemented for event type {}", __func__, libvmiEvent->type));
            }
        }
    }

    addr_t Event::getGfn()
    {
        switch (libvmiEvent->type)
        {
            case VMI_EVENT_INTERRUPT:
            {
                if (libvmiEvent->interrupt_event.intr != INT3)
                {
                    throw std::runtime_error("Invalid union access");
                }
                return libvmiEvent->interrupt_event.gfn;
            }
            default:
            {
                throw std::runtime_error(
                    fmt::format("{} not implemented for event type {}", __func__, libvmiEvent->type));
            }
        }
    }

    addr_t Event::getOffset()
    {
        switch (libvmiEvent->type)
        {
            case VMI_EVENT_INTERRUPT:
            {
                if (libvmiEvent->interrupt_event.intr != INT3)
                {
                    throw std::runtime_error("Invalid union access");
                }
                return libvmiEvent->interrupt_event.offset;
            }
            default:
            {
                throw std::runtime_error(
                    fmt::format("{} not implemented for event type {}", __func__, libvmiEvent->type));
            }
        }
    }
}
