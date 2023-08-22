#include "SystemEventSupervisor.h"
#include "../../vmi/VmiException.h"
#include <fmt/core.h>
#include <utility>
#include <vmicore/callback.h>
#include <vmicore/filename.h>

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
          logger(this->loggingLib->newNamedLogger(FILENAME_STEM)),
          eventStream(std::move(eventStream))
    {
    }

    void SystemEventSupervisor::initialize()
    {
        activeProcessesSupervisor->initialize();
        systemProcess = activeProcessesSupervisor->getProcessInformationByPid(SYSTEM_PID);
        interruptEventSupervisor->initialize();
        startPspCallProcessNotifyRoutinesMonitoring();
        startKeBugCheck2Monitoring();
    }

    void SystemEventSupervisor::startPspCallProcessNotifyRoutinesMonitoring()
    {
        auto processNotifyFunctionVA = vmiInterface->translateKernelSymbolToVA("PspCallProcessNotifyRoutines");
        logger->debug("Obtained starting address of PspCallProcessNotifyRoutines",
                      {{"VA", fmt::format("{:#x}", processNotifyFunctionVA)}});
        auto notifyProcessCallbackFunction = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(pspCallProcessNotifyRoutinesCallback);

        notifyProcessInterruptEvent = interruptEventSupervisor->createBreakpoint(
            processNotifyFunctionVA, systemProcess, notifyProcessCallbackFunction, true);
    }

    void SystemEventSupervisor::startKeBugCheck2Monitoring()
    {
        auto bugCheckFunctionVA = vmiInterface->translateKernelSymbolToVA("KeBugCheck2");
        logger->debug("Obtained starting address of KeBugCheck2", {{"VA", fmt::format("{:#x}", bugCheckFunctionVA)}});
        auto bugCheckCallbackFunction = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(keBugCheck2Callback);

        bugCheckInterruptEvent = interruptEventSupervisor->createBreakpoint(
            bugCheckFunctionVA, systemProcess, bugCheckCallbackFunction, true);
    }

    BpResponse SystemEventSupervisor::pspCallProcessNotifyRoutinesCallback(IInterruptEvent& event)
    {
        auto eprocessBase = event.getRcx();
        bool isTerminationEvent = event.getR8() == 0;
        logger->debug(fmt::format("{} called", __func__),
                      {
                          {"_EPROCESS_base ", fmt::format("{:#x}", eprocessBase)},
                          {"terminationFlag ", isTerminationEvent},
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
                logger->warning("InvalidArgumentException", {{"exception", e.what()}});
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

    BpResponse SystemEventSupervisor::keBugCheck2Callback(IInterruptEvent& event)
    {
        auto bugCheckCode = event.getRcx();
        eventStream->sendBSODEvent(static_cast<int64_t>(bugCheckCode));
        logger->warning("BSOD detected!", {{"BugCheckCode", fmt::format("{:#x}", bugCheckCode)}});
        GlobalControl::endVmi = true;
        GlobalControl::postRunPluginAction = false;
        pluginSystem->unloadPlugins();
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
