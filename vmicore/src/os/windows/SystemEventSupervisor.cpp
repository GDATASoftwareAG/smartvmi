#include "SystemEventSupervisor.h"
#include "../../io/grpc/GRPCLogger.h"
#include "../../vmi/VmiException.h"
#include <fmt/core.h>
#include <utility>

namespace VmiCore::Windows
{
    SystemEventSupervisor::SystemEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                 std::shared_ptr<IPluginSystem> pluginSystem,
                                                 std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                                                 std::shared_ptr<IConfigParser> configInterface,
                                                 std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor,
                                                 std::shared_ptr<ILogging> loggingLib,
                                                 std::shared_ptr<IEventStream> eventStream)
        : vmiInterface(std::move(vmiInterface)),
          pluginSystem(std::move(pluginSystem)),
          activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
          configInterface(std::move(configInterface)),
          interruptEventSupervisor(std::move(interruptEventSupervisor)),
          loggingLib(std::move(loggingLib)),
          logger(NEW_LOGGER(this->loggingLib)),
          eventStream(std::move(eventStream))
    {
    }

    void SystemEventSupervisor::initialize()
    {
        activeProcessesSupervisor->initialize();
        interruptEventSupervisor->initialize();
        startPspCallProcessNotifyRoutinesMonitoring();
        startKeBugCheckExMonitoring();
    }

    void SystemEventSupervisor::startPspCallProcessNotifyRoutinesMonitoring()
    {
        auto processNotifyFunctionVA = vmiInterface->translateKernelSymbolToVA("PspCallProcessNotifyRoutines");
        logger->debug("Obtained starting address of PspCallProcessNotifyRoutines",
                      {logfield::create("VA", fmt::format("{:#x}", processNotifyFunctionVA))});
        auto notifyProcessCallbackFunction = IBreakpoint::createBreakpointCallback(
            weak_from_this(), &SystemEventSupervisor::pspCallProcessNotifyRoutinesCallback);

        notifyProcessInterruptEvent = interruptEventSupervisor->createBreakpoint(
            processNotifyFunctionVA, vmiInterface->convertPidToDtb(systemPid), notifyProcessCallbackFunction);
    }

    void SystemEventSupervisor::startKeBugCheckExMonitoring()
    {
        auto bugCheckFunctionVA = vmiInterface->translateKernelSymbolToVA("KeBugCheckEx");
        logger->debug("Obtained starting address of KeBugCheckEx",
                      {logfield::create("VA", fmt::format("{:#x}", bugCheckFunctionVA))});
        auto bugCheckCallbackFunction =
            IBreakpoint::createBreakpointCallback(weak_from_this(), &SystemEventSupervisor::keBugCheckExCallback);

        bugCheckInterruptEvent = interruptEventSupervisor->createBreakpoint(
            bugCheckFunctionVA, vmiInterface->convertPidToDtb(systemPid), bugCheckCallbackFunction);
    }

    BpResponse SystemEventSupervisor::pspCallProcessNotifyRoutinesCallback(IInterruptEvent& event)
    {
        auto eprocessBase = event.getRcx();
        bool isTerminationEvent = event.getR8() == 0;
        logger->debug(fmt::format("{} called", __func__),
                      {
                          logfield::create("_EPROCESS_base ", fmt::format("{:#x}", eprocessBase)),
                          logfield::create("terminationFlag ", isTerminationEvent),
                      });
        if (isTerminationEvent)
        {
            try
            {
                pluginSystem->passProcessTerminationEventToRegisteredPlugins(
                    activeProcessesSupervisor->getProcessInformationByBase(eprocessBase));
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
            pluginSystem->passProcessStartEventToRegisteredPlugins(
                activeProcessesSupervisor->getProcessInformationByBase(eprocessBase));
        }
        return BpResponse::Continue;
    }

    BpResponse SystemEventSupervisor::keBugCheckExCallback(IInterruptEvent& event)
    {
        auto bugCheckCode = event.getRcx();
        eventStream->sendBSODEvent(static_cast<int64_t>(bugCheckCode));
        logger->warning("BSOD detected!", {logfield::create("BugCheckCode", fmt::format("{:#x}", bugCheckCode))});
        GlobalControl::endVmi = true;
        GlobalControl::postRunPluginAction = false;
        pluginSystem->passShutdownEventToRegisteredPlugins();
        // deactivate the interrupt event because we are terminating immediately (no single stepping)
        return BpResponse::Deactivate;
    }

    void SystemEventSupervisor::teardown()
    {
        notifyProcessInterruptEvent->remove();
        bugCheckInterruptEvent->remove();
        interruptEventSupervisor->teardown();
    }
}
