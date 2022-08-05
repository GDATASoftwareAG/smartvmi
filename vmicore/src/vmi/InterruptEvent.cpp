#include "InterruptEvent.h"
#include "../GlobalControl.h"
#include "../io/grpc/GRPCLogger.h"
#include "../os/PagingDefinitions.h"
#include "VmiException.h"
#include <fmt/core.h>
#include <memory>
#include <utility>

namespace
{
    auto event = vmi_event_t{};
    std::map<addr_t, InterruptEvent*> interruptsByPA;
    const std::string loggerName = std::filesystem::path(__FILE__).filename().stem();
}

void InterruptEvent::initializeInterruptEventHandling(ILibvmiInterface& vmiInterface)
{
    SETUP_INTERRUPT_EVENT(&event, _defaultInterruptCallback);
    event.interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
    vmiInterface.registerEvent(event);
}

void InterruptEvent::clearInterruptEventHandling(ILibvmiInterface& vmiInterface)
{
    vmiInterface.pauseVm();
    if (vmiInterface.areEventsPending())
    {
        GlobalControl::logger()->warning("Pending events during interrupt event destruction",
                                         {logfield::create("logger", loggerName)});
    }

    for (auto const& [_addr, interruptEvent] : interruptsByPA)
    {
        interruptEvent->teardown();
    }

    interruptsByPA.clear();
    vmiInterface.clearEvent(event, false);
    vmiInterface.resumeVm();
}

InterruptEvent::InterruptEvent(std::shared_ptr<ILibvmiInterface> vmiInterface,
                               uint64_t targetPA,
                               std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                               std::unique_ptr<InterruptGuard> interruptGuard,
                               std::function<InterruptResponse(InterruptEvent&)> callbackFunction,
                               std::unique_ptr<ILogger> logger)
    : Event(std::move(vmiInterface), std::move(logger)),
      targetPA(targetPA),
      singleStepSupervisor(std::move(singleStepSupervisor)),
      interruptGuard(std::move(interruptGuard)),
      callbackFunction(std::move(callbackFunction))
{
}

void InterruptEvent::initialize()
{
    targetPAString = fmt::format("{:#x}", targetPA);
    singleStepCallbackFunction =
        SingleStepSupervisor::createSingleStepCallback(shared_from_this(), &InterruptEvent::singleStepCallback);
    vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
    vmiInterface->flushPageCache();
    storeOriginalValue();
    setupVmiInterruptEvent();
    enableEvent();
}

void InterruptEvent::teardown()
{
    disableEvent();
    interruptGuard->teardown();
}

InterruptEvent::~InterruptEvent()
{
    interruptsByPA.erase(targetPA);
}

void InterruptEvent::setupVmiInterruptEvent()
{
    if (interruptsByPA.find(targetPA) != interruptsByPA.end())
    {
        throw VmiException(
            fmt::format("{}: Interrupt already registered at this address: {}", __func__, targetPAString));
    }
    interruptsByPA[targetPA] = this;
}

void InterruptEvent::enableEvent()
{
    vmiInterface->write8PA(targetPA, INT3_BREAKPOINT);
    logger->debug("Enabled interrupt event", {logfield::create("targetPA", targetPAString)});
}

void InterruptEvent::disableEvent()
{
    vmiInterface->write8PA(targetPA, originalValue);
    logger->debug("Disabled interrupt event", {logfield::create("targetPA", targetPAString)});
}

uint64_t InterruptEvent::getRdi()
{
    return event.x86_regs->rdi;
}

uint64_t InterruptEvent::getRcx()
{
    return event.x86_regs->rcx;
}

uint64_t InterruptEvent::getR8()
{
    return event.x86_regs->r8;
}

void InterruptEvent::storeOriginalValue()
{
    originalValue = vmiInterface->read8PA(targetPA);
    logger->debug("Save original value",
                  {logfield::create("targetPA", targetPAString),
                   logfield::create("originalValue", fmt::format("{:#x}", originalValue))});
    if (originalValue == INT3_BREAKPOINT)
    {
        throw VmiException(fmt::format(
            "{}: InterruptEvent originalValue @ {} is already an INT3 breakpoint.", __func__, targetPAString));
    }
}

event_response_t InterruptEvent::_defaultInterruptCallback(__attribute__((unused)) vmi_instance_t vmi,
                                                           vmi_event_t* event)
{
    auto eventResponse = VMI_EVENT_RESPONSE_NONE;
    try
    {
        auto eventPA =
            (event->interrupt_event.gfn << PagingDefinitions::numberOfPageIndexBits) + event->interrupt_event.offset;
        auto interruptEventIterator = interruptsByPA.find(eventPA);
        if (interruptEventIterator != interruptsByPA.end())
        {
            eventResponse = interruptEventIterator->second->interruptCallback(event->vcpu_id);
            event->interrupt_event.reinject = DONT_REINJECT_INTERRUPT;
        }
        else
        {
            GlobalControl::logger()->debug(
                "Reinject interrupt into guest OS",
                {logfield::create("logger", loggerName), logfield::create("eventPA", fmt::format("{:#x}", eventPA))});
            event->interrupt_event.reinject = REINJECT_INTERRUPT;
        }
        event->interrupt_event.insn_length = 1;
    }
    catch (const std::exception& e)
    {
        GlobalControl::endVmi = true;
        GlobalControl::logger()->error(
            "Unexpected exception", {logfield::create("logger", loggerName), logfield::create("exception", e.what())});
        GlobalControl::eventStream()->sendErrorEvent(e.what());
    }
    return eventResponse;
}

event_response_t InterruptEvent::interruptCallback(uint32_t vcpuId)
{
    vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
    vmiInterface->flushPageCache();

    InterruptResponse response;
    try
    {
        response = callbackFunction(*this);
    }
    catch (const std::runtime_error& e)
    {
        throw std::runtime_error(
            fmt::format("{}: {} Target physical address = {}", __func__, e.what(), targetPAString));
    }
    disableEvent();
    if (response == InterruptResponse::Continue)
    {
        singleStepSupervisor->setSingleStepCallback(vcpuId, singleStepCallbackFunction);
    }
    return VMI_EVENT_RESPONSE_NONE;
}

void InterruptEvent::singleStepCallback(__attribute__((unused)) vmi_event_t* singleStepEvent)
{
    enableEvent();
}
