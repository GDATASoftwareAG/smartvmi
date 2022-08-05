#ifndef VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H
#define VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogger.h"
#include "../../plugins/PluginSystem.h"
#include "../../vmi/InterruptEvent.h"
#include "../../vmi/InterruptFactory.h"
#include "../../vmi/LibvmiInterface.h"
#include "../../vmi/SingleStepSupervisor.h"
#include "../IActiveProcessesSupervisor.h"
#include "../ISystemEventSupervisor.h"
#include <memory>

namespace Linux
{
    class SystemEventSupervisor : public std::enable_shared_from_this<SystemEventSupervisor>,
                                  public ISystemEventSupervisor
    {
      public:
        ~SystemEventSupervisor() override = default;

        SystemEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                              std::shared_ptr<IPluginSystem> pluginSystem,
                              std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                              std::shared_ptr<IConfigParser> configInterface,
                              std::shared_ptr<IInterruptFactory> interruptFactory,
                              std::shared_ptr<ILogging> loggingLib,
                              std::shared_ptr<IEventStream> eventStream);

        void initialize() override;

        InterruptEvent::InterruptResponse procForkConnectorCallback(InterruptEvent& interruptEvent);

        InterruptEvent::InterruptResponse procExecConnectorCallback(InterruptEvent& interruptEvent);

        InterruptEvent::InterruptResponse procExitConnectorCallback(InterruptEvent& interruptEvent);

        void teardown() override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IPluginSystem> pluginSystem;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IConfigParser> configInterface;
        [[maybe_unused]] std::shared_ptr<InterruptEvent> procForkConnectorEvent;
        [[maybe_unused]] std::shared_ptr<InterruptEvent> procExecConnectorEvent;
        [[maybe_unused]] std::shared_ptr<InterruptEvent> procExitConnectorEvent;
        std::shared_ptr<IInterruptFactory> interruptFactory;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;

        void startProcForkConnectorMonitoring();

        void startProcExecConnectorMonitoring();

        void startProcExitConnectorMonitoring();
    };
}

#endif // VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H
