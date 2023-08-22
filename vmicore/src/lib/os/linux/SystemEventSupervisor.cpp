#include "SystemEventSupervisor.h"
#include "Constants.h"
#include <fmt/core.h>
#include <utility>
#include <vmicore/callback.h>
#include <vmicore/filename.h>

namespace VmiCore::Linux
{
    SystemEventSupervisor::SystemEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                 std::shared_ptr<IPluginSystem> pluginSystem,
                                                 std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                                                 std::shared_ptr<IConfigParser> configInterface,
                                                 std::shared_ptr<IInterruptEventSupervisor> interruptFactory,
                                                 std::shared_ptr<ILogging> loggingLib,
                                                 std::shared_ptr<IEventStream> eventStream)
        : vmiInterface(std::move(vmiInterface)),
          pluginSystem(std::move(pluginSystem)),
          activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
          configInterface(std::move(configInterface)),
          interruptEventSupervisor(std::move(interruptFactory)),
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
        startProcForkConnectorMonitoring();
        startProcExecConnectorMonitoring();
        startProcExitConnectorMonitoring();
    }

    void SystemEventSupervisor::startProcForkConnectorMonitoring()
    {
        auto procForkConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_fork_connector");
        logger->debug("Obtained starting address of proc_fork_connector",
                      {{"VA", fmt::format("{:#x}", procForkConnectorVA)}});
        auto procForkConnectorCallback = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(procForkConnectorCallback);
        systemProcess = activeProcessesSupervisor->getProcessInformationByPid(SYSTEM_PID);
        procForkConnectorEvent = interruptEventSupervisor->createBreakpoint(
            procForkConnectorVA, systemProcess, procForkConnectorCallback, false);
    }

    void SystemEventSupervisor::startProcExecConnectorMonitoring()
    {
        auto procExecConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_exec_connector");
        logger->debug("Obtained starting address of proc_exec_connector",
                      {{"VA", fmt::format("{:#x}", procExecConnectorVA)}});
        auto procExecConnectorCallback = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(procExecConnectorCallback);
        systemProcess = activeProcessesSupervisor->getProcessInformationByPid(SYSTEM_PID);
        procExecConnectorEvent = interruptEventSupervisor->createBreakpoint(
            procExecConnectorVA, systemProcess, procExecConnectorCallback, false);
    }

    void SystemEventSupervisor::startProcExitConnectorMonitoring()
    {
        auto procExitConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_exit_connector");
        logger->debug("Obtained starting address of proc_exit_connector",
                      {{"VA", fmt::format("{:#x}", procExitConnectorVA)}});
        auto procExitConnectorCallback = VMICORE_SETUP_SAFE_MEMBER_CALLBACK(procExitConnectorCallback);
        systemProcess = activeProcessesSupervisor->getProcessInformationByPid(SYSTEM_PID);
        procExitConnectorEvent = interruptEventSupervisor->createBreakpoint(
            procExitConnectorVA, systemProcess, procExitConnectorCallback, false);
    }

    BpResponse SystemEventSupervisor::procForkConnectorCallback(IInterruptEvent& event)
    {
        auto taskStructBase = event.getRdi();

        activeProcessesSupervisor->addNewProcess(taskStructBase);
        pluginSystem->passProcessStartEventToRegisteredPlugins(
            activeProcessesSupervisor->getProcessInformationByBase(taskStructBase));

        return BpResponse::Continue;
    }

    BpResponse SystemEventSupervisor::procExecConnectorCallback(IInterruptEvent& event)
    {
        auto taskStructBase = event.getRdi();

        pluginSystem->passProcessTerminationEventToRegisteredPlugins(
            activeProcessesSupervisor->getProcessInformationByBase(taskStructBase));
        activeProcessesSupervisor->removeActiveProcess(taskStructBase);
        activeProcessesSupervisor->addNewProcess(taskStructBase);
        pluginSystem->passProcessStartEventToRegisteredPlugins(
            activeProcessesSupervisor->getProcessInformationByBase(taskStructBase));

        return BpResponse::Continue;
    }

    BpResponse SystemEventSupervisor::procExitConnectorCallback(IInterruptEvent& event)
    {
        auto taskStructBase = event.getRdi();

        pluginSystem->passProcessTerminationEventToRegisteredPlugins(
            activeProcessesSupervisor->getProcessInformationByBase(taskStructBase));
        activeProcessesSupervisor->removeActiveProcess(taskStructBase);

        return BpResponse::Continue;
    }

    void SystemEventSupervisor::teardown()
    {
        procForkConnectorEvent->remove();
        procExecConnectorEvent->remove();
        procExitConnectorEvent->remove();
        interruptEventSupervisor->teardown();
    }
}
