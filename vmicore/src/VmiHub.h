#ifndef VMICORE_VMIHUB_H
#define VMICORE_VMIHUB_H

#include "config/IConfigParser.h"
#include "io/IEventStream.h"
#include "io/ILogging.h"
#include "os/windows/SystemEventSupervisor.h"
#include "plugins/PluginSystem.h"
#include "vmi/LibvmiInterface.h"
#include <memory>

class VmiHub
{
  public:
    VmiHub(std::shared_ptr<IConfigParser> configInterface,
           std::shared_ptr<ILibvmiInterface> vmiInterface,
           std::shared_ptr<PluginSystem> pluginSystem,
           std::shared_ptr<SystemEventSupervisor> systemEventSupervisor,
           std::shared_ptr<ILogging> loggingLib,
           std::shared_ptr<IEventStream> eventStream);

    uint run();

  private:
    std::shared_ptr<IConfigParser> configInterface;
    std::shared_ptr<ILibvmiInterface> vmiInterface;
    std::shared_ptr<PluginSystem> pluginSystem;
    std::shared_ptr<SystemEventSupervisor> systemEventSupervisor;
    std::shared_ptr<ILogging> loggingLib;
    std::unique_ptr<ILogger> logger;
    std::shared_ptr<IEventStream> eventStream;

    void waitForEvents() const;

    void performShutdownPluginAction() const;
};

#endif // VMICORE_VMIHUB_H
