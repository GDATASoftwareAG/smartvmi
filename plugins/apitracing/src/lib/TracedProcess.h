#ifndef APITRACING_TRACEDPROCESS_H
#define APITRACING_TRACEDPROCESS_H

#include "FunctionHook.h"
#include "config/FunctionDefinitions.h"
#include "config/TracingDefinitions.h"
#include "os/ILibrary.h"
#include <map>
#include <vmicore/plugins/PluginInterface.h>

namespace ApiTracing
{
    class ITracedProcess
    {
      public:
        virtual ~ITracedProcess() = default;

        virtual void removeHooks() noexcept = 0;

        [[nodiscard]] virtual bool traceChildren() const = 0;

        [[nodiscard]] virtual TracingProfile getTracingProfile() = 0;

      protected:
        ITracedProcess() = default;
    };

    class TracedProcess : public ITracedProcess
    {
      public:
        TracedProcess(VmiCore::Plugin::PluginInterface* pluginInterface,
                      std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                      std::shared_ptr<ILibrary> library,
                      std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation,
                      TracingProfile tracingProfile);

        ~TracedProcess() override = default;

        void removeHooks() noexcept override;

        [[nodiscard]] bool traceChildren() const override;

        [[nodiscard]] TracingProfile getTracingProfile() override;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::shared_ptr<IFunctionDefinitions> functionDefinitions;
        std::shared_ptr<ILibrary> library;
        std::map<std::string, uint64_t, std::less<>> loadedModules;
        std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation;
        TracingProfile tracingProfile;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::vector<std::shared_ptr<FunctionHook>> hookList;

        void initLoadedModules();

        void injectHooks();

        [[nodiscard]] std::size_t numberOfFunctionsToTrace() const;
    };
}

#endif // APITRACING_TRACEDPROCESS_H
