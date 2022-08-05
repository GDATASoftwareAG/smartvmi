#include "SystemEventSupervisor.h"
#include "../../GlobalControl.h"
#include "../../io/ILogger.h"
#include <fmt/core.h>
#include <utility>

namespace Linux
{
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
        activeProcessesSupervisor->initialize();
        interruptFactory->initialize();
        startProcForkConnectorMonitoring();
        startProcExecConnectorMonitoring();
        startProcExitConnectorMonitoring();
    }

    void SystemEventSupervisor::startProcForkConnectorMonitoring()
    {
        auto procForkConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_fork_connector");
        logger->debug("Obtained starting address of proc_fork_connector",
                      {logfield::create("VA", fmt::format(":#x", procForkConnectorVA))});
        auto procForkConnectorCallback = InterruptEvent::createInterruptCallback(
            weak_from_this(), &SystemEventSupervisor::procForkConnectorCallback);
        procForkConnectorEvent = interruptFactory->createInterruptEvent("procForkConnectorEvent",
                                                                        procForkConnectorVA,
                                                                        vmiInterface->convertPidToDtb(systemPid),
                                                                        procForkConnectorCallback);
    }

    void SystemEventSupervisor::startProcExecConnectorMonitoring()
    {
        auto procExecConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_exec_connector");
        logger->debug("Obtained starting address of proc_exec_connector",
                      {logfield::create("VA", fmt::format(":#x", procExecConnectorVA))});
        auto procExecConnectorCallback = InterruptEvent::createInterruptCallback(
            weak_from_this(), &SystemEventSupervisor::procExecConnectorCallback);
        procExecConnectorEvent = interruptFactory->createInterruptEvent("procExecConnectorEvent",
                                                                        procExecConnectorVA,
                                                                        vmiInterface->convertPidToDtb(systemPid),
                                                                        procExecConnectorCallback);
    }

    void SystemEventSupervisor::startProcExitConnectorMonitoring()
    {
        auto procExitConnectorVA = vmiInterface->translateKernelSymbolToVA("proc_exit_connector");
        logger->debug("Obtained starting address of proc_exit_connector",
                      {logfield::create("VA", fmt::format(":#x", procExitConnectorVA))});
        auto procExitConnectorCallback = InterruptEvent::createInterruptCallback(
            weak_from_this(), &SystemEventSupervisor::procExitConnectorCallback);
        procExitConnectorEvent = interruptFactory->createInterruptEvent("procExitConnectorEvent",
                                                                        procExitConnectorVA,
                                                                        vmiInterface->convertPidToDtb(systemPid),
                                                                        procExitConnectorCallback);
    }

    InterruptEvent::InterruptResponse SystemEventSupervisor::procForkConnectorCallback(InterruptEvent& interruptEvent)
    {
        activeProcessesSupervisor->addNewProcess(interruptEvent.getRdi());
        return InterruptEvent::InterruptResponse::Continue;
    }

    InterruptEvent::InterruptResponse SystemEventSupervisor::procExecConnectorCallback(InterruptEvent& interruptEvent)
    {
        activeProcessesSupervisor->removeActiveProcess(interruptEvent.getRdi());
        activeProcessesSupervisor->addNewProcess(interruptEvent.getRdi());
        return InterruptEvent::InterruptResponse::Continue;
    }

    InterruptEvent::InterruptResponse SystemEventSupervisor::procExitConnectorCallback(InterruptEvent& interruptEvent)
    {
        activeProcessesSupervisor->removeActiveProcess(interruptEvent.getRdi());
        return InterruptEvent::InterruptResponse::Continue;
    }

    void SystemEventSupervisor::teardown()
    {
        interruptFactory->teardown();
    }
}
