#include "InterruptEventSupervisor.h"
#include "Event.h"
#include "InterruptGuard.h"
#include "VmiException.h"
#include <memory>
#include <vmicore/callback.h>
#include <vmicore/filename.h>
#include <vmicore/os/PagingDefinitions.h>

namespace VmiCore
{

    namespace
    {
        InterruptEventSupervisor* interruptEventSupervisor = nullptr;
        constexpr auto loggerName = FILENAME_STEM;
    }

    InterruptEventSupervisor::InterruptEventSupervisor(
        std::shared_ptr<ILibvmiInterface> vmiInterface,
        std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
        std::shared_ptr<IRegisterEventSupervisor> registerEventSupervisor,
        std::shared_ptr<ILogging> loggingLib)
        : vmiInterface(std::move(vmiInterface)),
          singleStepSupervisor(std::move(singleStepSupervisor)),
          activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
          registerEventSupervisor(std::move(registerEventSupervisor)),
          loggingLib(std::move(loggingLib)),
          logger(this->loggingLib->newNamedLogger(loggerName))
    {
        interruptEventSupervisor = this;
    }

    InterruptEventSupervisor::~InterruptEventSupervisor() noexcept
    {
        interruptEventSupervisor = nullptr;
    }

    void InterruptEventSupervisor::initialize()
    {
        SETUP_INTERRUPT_EVENT(event, _defaultInterruptCallback);
        event->interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
        event->interrupt_event.insn_length = 1;
        vmiInterface->registerEvent(*event);
        singleStepSupervisor->initializeSingleStepEvents();
        singleStepCallbackFunction = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(singleStepCallback);
        contextSwitchCallbackFunction = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(contextSwitchCallback);
        registerEventSupervisor->setContextSwitchCallback(contextSwitchCallbackFunction);
    }

    void InterruptEventSupervisor::teardown()
    {
        clearInterruptEventHandling();
        singleStepSupervisor->teardown();
        registerEventSupervisor->teardown();
    }

    std::shared_ptr<IBreakpoint>
    InterruptEventSupervisor::createBreakpoint(uint64_t targetVA,
                                               const ActiveProcessInformation& processInformation,
                                               const std::function<BpResponse(IInterruptEvent&)>& callbackFunction,
                                               bool global)
    {
        auto processDtb = targetVA >= PagingDefinitions::kernelspaceLowerBoundary ? processInformation.processDtb
                                                                                  : processInformation.processUserDtb;
        auto targetPA = vmiInterface->convertVAToPA(targetVA, processDtb);
        auto targetGFN = targetPA >> PagingDefinitions::numberOfPageIndexBits;
        auto breakpoint = std::make_shared<Breakpoint>(
            targetPA,
            [supervisor = weak_from_this()](Breakpoint* bp)
            {
                if (auto supervisorShared = supervisor.lock())
                {
                    supervisorShared->deleteBreakpoint(bp);
                }
            },
            callbackFunction,
            processDtb,
            global);

        std::scoped_lock guard(lock);
        auto bpPage = breakpointsByGFN.find(targetGFN);
        // Check if there already is an interrupt registered on this page
        if (bpPage == breakpointsByGFN.end())
        {
            bpPage =
                breakpointsByGFN
                    .insert({targetGFN,
                             BpPage{.Breakpoints = {},
                                    // One PageGuard guards a whole memory page on which several interrupts may reside
                                    .PageGuard = createPageGuard(targetVA, processDtb, targetGFN)}})
                    .first;
        }

        // Register new INT3
        if (!bpPage->second.Breakpoints.contains(targetPA))
        {
            vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
            vmiInterface->flushPageCache();
            storeOriginalValue(targetPA);
            enableEvent(targetPA);

            bpPage->second.Breakpoints[targetPA] = std::vector<std::shared_ptr<Breakpoint>>{breakpoint};
        }
        // If there is already an interrupt registered to that PA, simply add the event to management
        else
        {
            bpPage->second.Breakpoints.at(targetPA).push_back(breakpoint);
            // The already registered interrupt is for another process
            if (paToBreakpointStatus.at(targetPA) == BPStateResponse::Disable)
            {
                enableEvent(targetPA);
            }
        }
        return breakpoint;
    }

    void InterruptEventSupervisor::deleteBreakpoint(IBreakpoint* breakpoint)
    {
        std::scoped_lock guard(lock);

        auto targetPA = breakpoint->getTargetPA();
        auto breakpointsAtGFN = breakpointsByGFN.find(targetPA >> PagingDefinitions::numberOfPageIndexBits);
        if (breakpointsAtGFN == breakpointsByGFN.end())
        {
            logger->warning("Breakpoint not found",
                            {{"Function", std::source_location::current().function_name()}, {"PA", targetPA}});
            vmiInterface->resumeVm();
            return;
        }
        auto breakpointsAtPA = breakpointsAtGFN->second.Breakpoints.find(targetPA);

        eraseBreakpointAtAddress(breakpointsAtPA->second, breakpoint);
        if (breakpointsAtPA->second.empty())
        {
            breakpointsAtGFN->second.Breakpoints.erase(breakpointsAtPA);

            if (vmiInterface->areEventsPending())
            {
                logger->debug(fmt::format("{}: Process pending events before removing breakpoint", __func__));
                // Make sure that all pending events are processed, so we don't receive interrupt events for removed
                // breakpoints
                vmiInterface->eventsListen(0);
            }

            removeInterrupt(targetPA);
            if (breakpointsAtGFN->second.Breakpoints.empty())
            {
                breakpointsAtGFN->second.PageGuard->teardown();
                breakpointsByGFN.erase(breakpointsAtGFN);
            }
        }
    }

    void InterruptEventSupervisor::enableEvent(addr_t targetPA)
    {
        vmiInterface->write8PA(targetPA, INT3_BREAKPOINT);
        paToBreakpointStatus[targetPA] = BPStateResponse::Enable;
        GlobalControl::logger()->debug("Enabled interrupt event", {{"targetPA", fmt::format("{:#x}", targetPA)}});
    }

    void InterruptEventSupervisor::disableEvent(addr_t targetPA)
    {
        vmiInterface->write8PA(targetPA, originalValuesByTargetPA[targetPA]);
        paToBreakpointStatus[targetPA] = BPStateResponse::Disable;
        GlobalControl::logger()->debug("Disabled interrupt event", {{"targetPA", fmt::format("{:#x}", targetPA)}});
    }

    event_response_t InterruptEventSupervisor::_defaultInterruptCallback([[maybe_unused]] vmi_instance_t vmi,
                                                                         vmi_event_t* event)
    {
        auto eventResponse = VMI_EVENT_RESPONSE_NONE;
        event->interrupt_event.reinject = REINJECT_INTERRUPT;

        // TODO: fix event gfn in kvmi
        //        auto eventPA =
        //            (event->interrupt_event.gfn << PagingDefinitions::numberOfPageIndexBits) +
        //            event->interrupt_event.offset;
        if (interruptEventSupervisor == nullptr)
        {
            GlobalControl::logger()->error(
                "Caught interrupt event with destroyed InterruptEventSupervisor",
                {CxxLogField("logger", loggerName) /*, CxxLogField("eventPA", fmt::format("{:#x}", eventPA))*/});
            return eventResponse;
        }
        interruptEventSupervisor->vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
        auto eventPA =
            interruptEventSupervisor->vmiInterface->convertVAToPA(event->interrupt_event.gla, event->x86_regs->cr3);
        auto breakpointsAtEventGFN =
            interruptEventSupervisor->breakpointsByGFN.find(eventPA >> PagingDefinitions::numberOfPageIndexBits);
        if (breakpointsAtEventGFN != interruptEventSupervisor->breakpointsByGFN.end())
        {
            auto breakpointsAtEventPA = breakpointsAtEventGFN->second.Breakpoints.find(eventPA);
            if (breakpointsAtEventPA != breakpointsAtEventGFN->second.Breakpoints.end())
            {
                event->interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
                return interruptEventSupervisor->interruptCallback(
                    eventPA, event->vcpu_id, breakpointsAtEventPA->second);
            }
        }

        if (event->interrupt_event.reinject == REINJECT_INTERRUPT)
        {
            GlobalControl::logger()->debug("Reinject interrupt into guest OS",
                                           {{"logger", loggerName}, {"eventPA", fmt::format("{:#x}", eventPA)}});
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
                GlobalControl::logger()->warning("Interrupt callback failed",
                                                 {{"logger", loggerName}, {"exception", e.what()}});
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

    void InterruptEventSupervisor::contextSwitchCallback(vmi_event_t* registerEvent)
    {
        auto newDtb = registerEvent->reg_event.value;

        for (const auto& [_bpPageGFN, bpPage] : interruptEventSupervisor->breakpointsByGFN)
        {
            for (const auto& [breakpointsPA, breakpoints] : bpPage.Breakpoints)
            {
                BPStateResponse currentBreakpointState = paToBreakpointStatus.at(breakpointsPA);

                auto newBreakpointState = getNewBreakpointState(newDtb, breakpoints);

                if (newBreakpointState != currentBreakpointState)
                {
                    paToBreakpointStatus.at(breakpointsPA) = newBreakpointState;
                    interruptEventSupervisor->updateBreakpointState(newBreakpointState, breakpointsPA);
                }
            }
        }
    }

    std::shared_ptr<InterruptGuard>
    InterruptEventSupervisor::createPageGuard(uint64_t targetVA, uint64_t processDtb, uint64_t targetGFN)
    {
        auto interruptGuard =
            std::make_shared<InterruptGuard>(vmiInterface, loggingLib, targetVA, targetGFN, processDtb);
        interruptGuard->initialize();

        return interruptGuard;
    }

    void InterruptEventSupervisor::storeOriginalValue(addr_t targetPA)
    {
        auto originalValue = vmiInterface->read8PA(targetPA);
        GlobalControl::logger()->debug(
            "Save original value",
            {{"targetPA", fmt::format("{:#x}", targetPA)}, {"originalValue", fmt::format("{:#x}", originalValue)}});
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

        for (const auto& [_gfn, breakpointsAtGfn] : breakpointsByGFN)
        {
            for (const auto& [breakpointPA, _breakpointsAtPa] : breakpointsAtGfn.Breakpoints)
            {
                removeInterrupt(breakpointPA);
            }
            breakpointsAtGfn.PageGuard->teardown();
        }

        breakpointsByGFN.clear();
        vmiInterface->clearEvent(*event, false);

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
        paToBreakpointStatus.erase(targetPA);
    }

    BPStateResponse
    InterruptEventSupervisor::getNewBreakpointState(reg_t newDtb,
                                                    const std::vector<std::shared_ptr<Breakpoint>>& breakpoints)
    {
        using enum BPStateResponse;
        for (const auto& breakpoint : breakpoints)
        {
            auto newState = breakpoint->getNewBreakpointState(newDtb);
            if (newState == Enable) // if at least one bp needs to be enabled, enable it
            {
                return Enable;
            }
        }
        return Disable;
    }

    void InterruptEventSupervisor::updateBreakpointState(BPStateResponse state, uint64_t targetPA) const
    {
        using enum BPStateResponse;
        switch (state)
        {
            case Enable:
            {
                interruptEventSupervisor->enableEvent(targetPA);
                break;
            }
            case Disable:
            {
                interruptEventSupervisor->disableEvent(targetPA);
                break;
            }
            default:
            {
                throw VmiException(fmt::format("Invalid breakpoint state"));
            }
        }
    }
}
