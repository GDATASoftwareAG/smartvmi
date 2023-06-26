#ifndef APITRACING_TRACER_H
#define APITRACING_TRACER_H

#include "TracedProcessFactory.h"
#include "config/Config.h"
#include "os/ILibrary.h"
#include <map>
#include <memory>
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/PluginInterface.h>

namespace ApiTracing
{
    class Tracer
    {
      public:
        Tracer(VmiCore::Plugin::PluginInterface* pluginInterface,
               std::unique_ptr<IConfig> config,
               std::shared_ptr<ITracedProcessFactory> tracedProcessFactory);

        void traceProcess(const std::shared_ptr<const VmiCore::ActiveProcessInformation>& processInformation);

        void removeTracedProcess(const std::shared_ptr<const VmiCore::ActiveProcessInformation>& processInformation);

        void teardown() noexcept;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::unique_ptr<IConfig> config;
        std::map<pid_t, std::unique_ptr<ITracedProcess>> tracedProcesses;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::shared_ptr<ITracedProcessFactory> tracedProcessFactory;

        [[nodiscard]] std::optional<TracingProfile>
        getProcessTracingProfile(const VmiCore::ActiveProcessInformation& processInformation) const;
    };
}
#endif // APITRACING_TRACER_H
