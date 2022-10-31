#include "InterruptEventSupervisor.h"
#include "../os/PagingDefinitions.h"
#include "Event.h"
#include "InterruptGuard.h"
#include "VmiException.h"
#include <memory>

namespace VmiCore
{
    namespace
    {
        InterruptEventSupervisor* interruptEventSupervisor = nullptr;
        const std::string loggerName = std::filesystem::path(__FILE__).filename().stem();
    }

    InterruptEventSupervisor::InterruptEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                       std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                                                       std::shared_ptr<ILogging> loggingLib)
        : vmiInterface(std::move(vmiInterface)),
          singleStepSupervisor(std::move(singleStepSupervisor)),
          loggingLib(std::move(loggingLib)),
          logger(NEW_LOGGER(this->loggingLib))
    {
        interruptEventSupervisor = this;
    }

    InterruptEventSupervisor::~InterruptEventSupervisor() noexcept
    {
        interruptEventSupervisor = nullptr;
    }

    void InterruptEventSupervisor::initialize()
    {
        SETUP_INTERRUPT_EVENT(&event, _defaultInterruptCallback);
        event.interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
        event.interrupt_event.insn_length = 1;
        vmiInterface->registerEvent(event);
        singleStepSupervisor->initializeSingleStepEvents();
        singleStepCallbackFunction = SingleStepSupervisor::createSingleStepCallback(
            shared_from_this(), &InterruptEventSupervisor::singleStepCallback);
    }

    void InterruptEventSupervisor::teardown()
    {
        clearInterruptEventHandling();
        singleStepSupervisor->teardown();
    }

    std::shared_ptr<IBreakpoint> InterruptEventSupervisor::createBreakpoint(
        uint64_t targetVA, uint64_t processDtb, const std::function<BpResponse(IInterruptEvent&)>& callbackFunction)
    {

        auto targetPA = vmiInterface->convertVAToPA(targetVA, processDtb);
        auto breakpoint = std::make_shared<Breakpoint>(
            targetPA,
            [supervisor = weak_from_this()](Breakpoint* bp)
            {
                if (auto supervisorShared = supervisor.lock())
                {
                    supervisorShared->deleteBreakpoint(bp);
                }
            },
            callbackFunction);

        std::scoped_lock guard(lock);
        // Register new INT3
        if (!breakpointsByPA.contains(targetPA))
        {
            vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
            vmiInterface->flushPageCache();
            storeOriginalValue(targetPA);
            enableEvent(targetPA);

            auto targetGFN = targetPA >> PagingDefinitions::numberOfPageIndexBits;
            // One PageGuard guards a whole memory page on which several interrupts may reside.
            if (!interruptGuardsByGFN.contains(targetGFN))
            {
                interruptGuardsByGFN[targetGFN] = createPageGuard(targetVA, processDtb, targetGFN);
            }

            breakpointsByPA[targetPA] = std::vector<std::shared_ptr<Breakpoint>>{breakpoint};
        }
        // If there is already an interrupt registered to that PA, simply add the event to management
        else
        {
            breakpointsByPA.at(targetPA).push_back(breakpoint);
        }

        return breakpoint;
    }

    void InterruptEventSupervisor::deleteBreakpoint(IBreakpoint* breakpoint)
    {
        std::scoped_lock guard(lock);

        auto targetPA = breakpoint->getTargetPA();
        auto breakpointsAtPA = breakpointsByPA.find(targetPA);
        eraseBreakpointAtAddress(breakpointsAtPA->second, breakpoint);
        if (breakpointsAtPA->second.empty())
        {
            // Prevent new events from happening while we are removing the INT3 from memory
            vmiInterface->pauseVm();
            if (vmiInterface->areEventsPending())
            {
                logger->debug(fmt::format("{}: Process pending events before removing breakpoint", __func__));
                // Make sure that all pending events are processed, so we don't receive interrupt events for removed
                // breakpoints
                vmiInterface->eventsListen(0);
            }
            removeInterrupt(targetPA);
            vmiInterface->resumeVm();

            breakpointsByPA.erase(breakpointsAtPA);
        }
    }

    void InterruptEventSupervisor::enableEvent(addr_t targetPA)
    {
        vmiInterface->write8PA(targetPA, INT3_BREAKPOINT);
        GlobalControl::logger()->debug("Enabled interrupt event",
                                       {logfield::create("targetPA", fmt::format("{:#x}", targetPA))});
    }

    void InterruptEventSupervisor::disableEvent(addr_t targetPA)
    {
        vmiInterface->write8PA(targetPA, originalValuesByTargetPA[targetPA]);
        GlobalControl::logger()->debug("Disabled interrupt event",
                                       {logfield::create("targetPA", fmt::format("{:#x}", targetPA))});
    }

    event_response_t InterruptEventSupervisor::_defaultInterruptCallback([[maybe_unused]] vmi_instance_t vmi,
                                                                         vmi_event_t* event)
    {
        auto eventResponse = VMI_EVENT_RESPONSE_NONE;

        // TODO: fix event gfn in kvmi
        //        auto eventPA =
        //            (event->interrupt_event.gfn << PagingDefinitions::numberOfPageIndexBits) +
        //            event->interrupt_event.offset;
        if (interruptEventSupervisor == nullptr)
        {
            GlobalControl::logger()->error(
                "Caught interrupt event with destroyed InterruptEventSupervisor",
                {logfield::create("logger",
                                  loggerName) /*, logfield::create("eventPA", fmt::format("{:#x}", eventPA))*/});
            return eventResponse;
        }
        auto eventPA =
            interruptEventSupervisor->vmiInterface->convertVAToPA(event->interrupt_event.gla, event->x86_regs->cr3);

        auto breakpointsAtEventPA = interruptEventSupervisor->breakpointsByPA.find(eventPA);
        if (breakpointsAtEventPA != interruptEventSupervisor->breakpointsByPA.end())
        {
            event->interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
            interruptEventSupervisor->interruptCallback(eventPA, event->vcpu_id, breakpointsAtEventPA->second);
        }
        else
        {
            GlobalControl::logger()->debug(
                "Reinject interrupt into guest OS",
                {logfield::create("logger", loggerName), logfield::create("eventPA", fmt::format("{:#x}", eventPA))});
            event->interrupt_event.reinject = REINJECT_INTERRUPT;
        }

        return eventResponse;
    }

    event_response_t InterruptEventSupervisor::interruptCallback(
        addr_t interruptPA, uint32_t vcpuId, const std::vector<std::shared_ptr<Breakpoint>>& breakpoints)
    {
        bool deactivateInterrupt = false;

        vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
        vmiInterface->flushPageCache();

        for (auto& breakpoint : breakpoints)
        {
            try
            {
                auto eventResponse = breakpoint->callback(interruptEvent);
                if (eventResponse == BpResponse::Deactivate)
                {
                    deactivateInterrupt = true;
                }
            }
            catch (const std::exception& e)
            {
                GlobalControl::logger()->warning(
                    "Interrupt callback failed",
                    {logfield::create("logger", loggerName), logfield::create("exception", e.what())});
                GlobalControl::eventStream()->sendErrorEvent(e.what());
            }
        }

        disableEvent(interruptPA);

        if (!deactivateInterrupt)
        {
            singleStepSupervisor->setSingleStepCallback(vcpuId, singleStepCallbackFunction, interruptPA);
        }

        return VMI_EVENT_RESPONSE_NONE;
    }

    void InterruptEventSupervisor::singleStepCallback(vmi_event_t* singleStepEvent)
    {
        enableEvent(reinterpret_cast<addr_t>(singleStepEvent->data));
    }

    std::unique_ptr<InterruptGuard>
    InterruptEventSupervisor::createPageGuard(uint64_t targetVA, uint64_t processDtb, uint64_t targetGFN)
    {
        auto interruptGuard =
            std::make_unique<InterruptGuard>(vmiInterface, loggingLib, targetVA, targetGFN, processDtb);

        interruptGuard->initialize();
        return interruptGuard;
    }

    void InterruptEventSupervisor::storeOriginalValue(addr_t targetPA)
    {
        auto originalValue = vmiInterface->read8PA(targetPA);
        GlobalControl::logger()->debug("Save original value",
                                       {logfield::create("targetGFN", fmt::format("{:#x}", targetPA)),
                                        logfield::create("originalValue", fmt::format("{:#x}", originalValue))});
        if (originalValue == INT3_BREAKPOINT)
        {
            throw VmiException(
                fmt::format("{}: Breakpoint originalValue @ {:#x} is already an INT3 breakpoint.", __func__, targetPA));
        }

        originalValuesByTargetPA[targetPA] = originalValue;
    }

    void InterruptEventSupervisor::clearInterruptEventHandling()
    {
        vmiInterface->pauseVm();

        for (const auto& breakpointsAtPA : breakpointsByPA)
        {
            removeInterrupt(breakpointsAtPA.first);
        }
        breakpointsByPA.clear();

        vmiInterface->clearEvent(event, false);
        vmiInterface->resumeVm();
    }

    void
    InterruptEventSupervisor::eraseBreakpointAtAddress(std::vector<std::shared_ptr<Breakpoint>>& breakpointsAtAddress,
                                                       const IBreakpoint* breakpoint)
    {
        breakpointsAtAddress.erase(std::find_if(breakpointsAtAddress.cbegin(),
                                                breakpointsAtAddress.cend(),
                                                [breakpoint](auto sharedInterruptEventPtr)
                                                { return sharedInterruptEventPtr.get() == breakpoint; }));
    }

    void InterruptEventSupervisor::removeInterrupt(addr_t targetPA)
    {
        auto originalValue = originalValuesByTargetPA.extract(targetPA);
        vmiInterface->write8PA(targetPA, originalValue.mapped());

        auto targetGFN = targetPA >> PagingDefinitions::numberOfPageIndexBits;
        auto interruptGuard = interruptGuardsByGFN.extract(targetGFN);
        interruptGuard.mapped()->teardown();
    }
}
