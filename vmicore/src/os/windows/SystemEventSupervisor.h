#ifndef VMICORE_SYSTEMEVENTSUPERVISOR_H
#define VMICORE_SYSTEMEVENTSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../plugins/PluginSystem.h"
#include "../../vmi/InterruptEvent.h"
#include "../../vmi/InterruptFactory.h"
#include "../../vmi/LibvmiInterface.h"
#include "../../vmi/SingleStepSupervisor.h"
#include "ActiveProcessesSupervisor.h"
#include <memory>

class SystemEventSupervisor : public std::enable_shared_from_this<SystemEventSupervisor>
{
  public:
    SystemEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                          std::shared_ptr<IPluginSystem> pluginSystem,
                          std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                          std::shared_ptr<IConfigParser> configInterface,
                          std::shared_ptr<IInterruptFactory> interruptFactory,
                          std::shared_ptr<ILogging> loggingLib,
                          std::shared_ptr<IEventStream> eventStream);

    void initialize();

    InterruptEvent::InterruptResponse pspCallProcessNotifyRoutinesCallback(InterruptEvent& interruptEvent);

    InterruptEvent::InterruptResponse keBugCheckExCallback(InterruptEvent& interruptEvent);

    void teardown();

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
    uint64_t systemCr3{};

    void startPspCallProcessNotifyRoutinesMonitoring();

    void startKeBugCheckExMonitoring();
};

#endif // VMICORE_SYSTEMEVENTSUPERVISOR_H
