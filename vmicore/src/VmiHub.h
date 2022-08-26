#ifndef VMICORE_VMIHUB_H
#define VMICORE_VMIHUB_H

#include "config/IConfigParser.h"
#include "io/IEventStream.h"
#include "io/ILogging.h"
#include "os/ISystemEventSupervisor.h"
#include "plugins/PluginSystem.h"
#include "vmi/InterruptFactory.h"
#include "vmi/LibvmiInterface.h"
#include <memory>
#include <unordered_map>

namespace VmiCore
{
    class VmiHub
    {
      public:
        VmiHub(std::shared_ptr<IConfigParser> configInterface,
               std::shared_ptr<ILibvmiInterface> vmiInterface,
               std::shared_ptr<ILogging> loggingLib,
               std::shared_ptr<IEventStream> eventStream,
               std::shared_ptr<IInterruptFactory> interruptFactory);

        uint run(const std::unordered_map<std::string, std::vector<std::string>>& pluginArgs);

      private:
        std::shared_ptr<IConfigParser> configInterface;
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<PluginSystem> pluginSystem;
        std::shared_ptr<ISystemEventSupervisor> systemEventSupervisor;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;
        std::shared_ptr<IInterruptFactory> interruptFactory;

        void waitForEvents() const;

        void performShutdownPluginAction() const;
    };
}

#endif // VMICORE_VMIHUB_H
