#include "SystemEventSupervisor.h"
#include "../../GlobalControl.h"
#include "../../io/grpc/GRPCLogger.h"
#include "../../vmi/VmiException.h"
#include <utility>

SystemEventSupervisor::SystemEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                             std::shared_ptr<IPluginSystem> pluginSystem,
                                             std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                                             std::shared_ptr<IConfigParser> configInterface,
                                             std::shared_ptr<IInterruptFactory> interruptFactory,
                                             std::shared_ptr<ILogging> loggingLib,
                                             std::shared_ptr<IEventStream> eventStream)
    : vmiInterface(std::move(vmiInterface)),
      pluginSystem(std::move(pluginSystem)),
      activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
      configInterface(std::move(configInterface)),
      interruptFactory(std::move(interruptFactory)),
      loggingLib(std::move(loggingLib)),
      logger(NEW_LOGGER(this->loggingLib)),
      eventStream(std::move(eventStream))
{
}

void SystemEventSupervisor::initialize()
{
    systemCr3 = vmiInterface->getSystemCr3();
    activeProcessesSupervisor->initialize();
    interruptFactory->initialize();
    startPspCallProcessNotifyRoutinesMonitoring();
    startKeBugCheckExMonitoring();
}

void SystemEventSupervisor::startPspCallProcessNotifyRoutinesMonitoring()
{
    auto processNotifyFunctionVA = vmiInterface->translateKernelSymbolToVA("PspCallProcessNotifyRoutines");
    logger->debug("Obtained starting address of PspCallProcessNotifyRoutines",
                  {logfield::create("VA", Convenience::intToHex(processNotifyFunctionVA))});
    auto notifyProcessCallbackFunction = InterruptEvent::createInterruptCallback(
        weak_from_this(), &SystemEventSupervisor::pspCallProcessNotifyRoutinesCallback);

    notifyProcessInterruptEvent = interruptFactory->createInterruptEvent("PspCallProcessNotifyRoutinesInterruptEvent",
                                                                         processNotifyFunctionVA,
                                                                         systemCr3,
                                                                         notifyProcessCallbackFunction);
}

void SystemEventSupervisor::startKeBugCheckExMonitoring()
{
    auto bugCheckFunctionVA = vmiInterface->translateKernelSymbolToVA("KeBugCheckEx");
    logger->debug("Obtained starting address of KeBugCheckEx",
                  {logfield::create("VA", Convenience::intToHex(bugCheckFunctionVA))});
    auto bugCheckCallbackFunction =
        InterruptEvent::createInterruptCallback(weak_from_this(), &SystemEventSupervisor::keBugCheckExCallback);

    bugCheckInterruptEvent = interruptFactory->createInterruptEvent(
        "KeBugCheckExInterruptEvent", bugCheckFunctionVA, systemCr3, bugCheckCallbackFunction);
}

InterruptEvent::InterruptResponse
SystemEventSupervisor::pspCallProcessNotifyRoutinesCallback(InterruptEvent& interruptEvent)
{
    auto eprocessBase = interruptEvent.getRcx();
    bool isTerminationEvent = interruptEvent.getR8() == 0;
    logger->debug("Called",
                  {
                      logfield::create("Function", static_cast<std::string>(__func__)),
                      logfield::create("_EPROCESS_base ", Convenience::intToHex(eprocessBase)),
                      logfield::create("terminationFlag ", isTerminationEvent),
                  });
    if (isTerminationEvent)
    {
        try
        {
            auto processInformation = activeProcessesSupervisor->getProcessInformationByEprocessBase(eprocessBase);
            pluginSystem->passProcessTerminationEventToRegisteredPlugins(processInformation->pid,
                                                                         *processInformation->fullName);
            activeProcessesSupervisor->removeActiveProcess(eprocessBase);
        }
        catch (const std::invalid_argument& e)
        {
            logger->warning("InvalidArgumentException", {logfield::create("exception", e.what())});
        }
    }
    else
    {
        activeProcessesSupervisor->addNewProcess(eprocessBase);
    }
    return InterruptEvent::InterruptResponse::Continue;
}

InterruptEvent::InterruptResponse SystemEventSupervisor::keBugCheckExCallback(InterruptEvent& interruptEvent)
{
    auto bugCheckCode = interruptEvent.getRcx();
    eventStream->sendBSODEvent(static_cast<int64_t>(bugCheckCode));
    logger->warning("BSOD detected!", {logfield::create("BugCheckCode", Convenience::intToHex(bugCheckCode))});
    GlobalControl::endVmi = true;
    GlobalControl::postRunPluginAction = false;
    pluginSystem->passShutdownEventToRegisteredPlugins();
    // deactivate the interrupt event because we are terminating immediately (no single stepping)
    return InterruptEvent::InterruptResponse::Deactivate;
}

void SystemEventSupervisor::teardown()
{
    interruptFactory->teardown();
}
