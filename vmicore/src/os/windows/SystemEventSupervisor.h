#ifndef VMICORE_WINDOWS_SYSTEMEVENTSUPERVISOR_H
#define VMICORE_WINDOWS_SYSTEMEVENTSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../plugins/PluginSystem.h"
#include "../../vmi/InterruptEvent.h"
#include "../../vmi/InterruptFactory.h"
#include "../../vmi/LibvmiInterface.h"
#include "../../vmi/SingleStepSupervisor.h"
#include "../ISystemEventSupervisor.h"
#include "ActiveProcessesSupervisor.h"
#include <memory>

namespace VmiCore::Windows
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

        InterruptEvent::InterruptResponse pspCallProcessNotifyRoutinesCallback(InterruptEvent& interruptEvent);

        InterruptEvent::InterruptResponse keBugCheckExCallback(InterruptEvent& interruptEvent);

        void teardown() override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IPluginSystem> pluginSystem;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IConfigParser> configInterface;
        [[maybe_unused]] std::shared_ptr<InterruptEvent> notifyProcessInterruptEvent;
        [[maybe_unused]] std::shared_ptr<InterruptEvent> bugCheckInterruptEvent;
        std::shared_ptr<IInterruptFactory> interruptFactory;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;

        void startPspCallProcessNotifyRoutinesMonitoring();

        void startKeBugCheckExMonitoring();
    };
}

#endif // VMICORE_WINDOWS_SYSTEMEVENTSUPERVISOR_H
