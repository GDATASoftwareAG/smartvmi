#ifndef VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H
#define VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogger.h"
#include "../../io/ILogging.h"
#include "../../plugins/PluginSystem.h"
#include "../../vmi/InterruptEventSupervisor.h"
#include "../../vmi/LibvmiInterface.h"
#include "../../vmi/SingleStepSupervisor.h"
#include "../IActiveProcessesSupervisor.h"
#include "../ISystemEventSupervisor.h"
#include <memory>

namespace VmiCore::Linux
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
                              std::shared_ptr<IInterruptEventSupervisor> interruptFactory,
                              std::shared_ptr<ILogging> loggingLib,
                              std::shared_ptr<IEventStream> eventStream);

        void initialize() override;

        BpResponse procForkConnectorCallback(IInterruptEvent& event);

        BpResponse procExecConnectorCallback(IInterruptEvent& event);

        BpResponse procExitConnectorCallback(IInterruptEvent& event);

        void teardown() override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IPluginSystem> pluginSystem;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IConfigParser> configInterface;
        std::shared_ptr<IBreakpoint> procForkConnectorEvent;
        std::shared_ptr<IBreakpoint> procExecConnectorEvent;
        std::shared_ptr<IBreakpoint> procExitConnectorEvent;
        std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;

        void startProcForkConnectorMonitoring();

        void startProcExecConnectorMonitoring();

        void startProcExitConnectorMonitoring();
    };
}

#endif // VMICORE_LINUX_SYSTEMEVENTSUPERVISOR_H
