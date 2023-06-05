#ifndef APITRACING_TRACER_H
#define APITRACING_TRACER_H

#include "ConstantDefinitions.h"
#include "FunctionHook.h"
#include "config/FunctionDefinitions.h"
#include "config/TracingDefinitions.h"
#include "config/TracingTargetsParser.h"
#include "os/ILibrary.h"
#include <map>
#include <memory>
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/PluginInterface.h>

namespace ApiTracing
{
    class Tracer : public std::enable_shared_from_this<Tracer>
    {
      public:
        Tracer(VmiCore::Plugin::PluginInterface* pluginInterface,
               const ITracingTargetsParser& tracingTargetsParser,
               std::shared_ptr<IFunctionDefinitions> functionDefinitions,
               std::shared_ptr<ILibrary> library);

        void initLoadedModules(const VmiCore::ActiveProcessInformation& processInformation);

        std::map<std::string, uint64_t, std::less<>> getLoadedModules() const;

        void addHooks(std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation);

        void removeHooks() const;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::map<std::string, uint64_t, std::less<>> loadedModules;
        std::map<pid_t, std::string> tracedProcesses;
        std::vector<ProcessTracingConfig> tracingTargetConfigs;
        std::shared_ptr<IFunctionDefinitions> functionDefinitions;
        std::vector<std::shared_ptr<FunctionHook>> hookList;
        std::map<uint64_t, std::map<uint64_t, std::list<ParameterInformation>>> processFunctionDefinitions{};
        std::shared_ptr<ILibrary> library;
        std::unique_ptr<VmiCore::ILogger> logger;

        void injectHooks(const VmiCore::ActiveProcessInformation& processInformation,
                         const std::optional<ProcessTracingConfig>& processTracingConfig);

        std::optional<ProcessTracingConfig>
        getProcessTracingConfig(const VmiCore::ActiveProcessInformation& processInformation) const;

        bool shouldProcessBeMonitored(const VmiCore::ActiveProcessInformation& processInformation,
                                      const std::optional<ProcessTracingConfig>& processTracingConfig) const;
    };
}
#endif // APITRACING_TRACER_H
