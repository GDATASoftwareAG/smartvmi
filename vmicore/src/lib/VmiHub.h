#ifndef VMICORE_VMIHUB_H
#define VMICORE_VMIHUB_H

#include "config/IConfigParser.h"
#include "io/IEventStream.h"
#include "io/ILogging.h"
#include "os/ISystemEventSupervisor.h"
#include "plugins/PluginSystem.h"
#include "vmi/InterruptEventSupervisor.h"
#include "vmi/LibvmiInterface.h"
#include "vmicore/io/ILogger.h"
#include <functional> // std::equal_to
#include <map>
#include <memory>

namespace VmiCore
{
    class VmiHub
    {
      public:
        VmiHub(std::shared_ptr<IConfigParser> configInterface,
               std::shared_ptr<ILibvmiInterface> vmiInterface,
               std::shared_ptr<ILogging> loggingLib,
               std::shared_ptr<IEventStream> eventStream,
               std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor,
               std::shared_ptr<IFileTransport> pluginTransport);

        uint run(const std::map<std::string, std::vector<std::string>, std::equal_to<>>& pluginArgs);

      private:
        std::shared_ptr<IConfigParser> configInterface;
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<PluginSystem> pluginSystem;
        std::shared_ptr<ISystemEventSupervisor> systemEventSupervisor;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;
        std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor;
        std::shared_ptr<IFileTransport> pluginTransport;

        void waitForEvents() const;
    };
}

#endif // VMICORE_VMIHUB_H
