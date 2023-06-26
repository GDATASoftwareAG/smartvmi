#ifndef APITRACING_TRACEDPROCESSFACTORY_H
#define APITRACING_TRACEDPROCESSFACTORY_H

#include "TracedProcess.h"
#include "config/FunctionDefinitions.h"
#include "os/ILibrary.h"
#include <memory>
#include <vmicore/os/ActiveProcessInformation.h>
#include <vmicore/plugins/PluginInterface.h>

namespace ApiTracing
{
    class ITracedProcessFactory
    {
      public:
        virtual ~ITracedProcessFactory() = default;

        [[nodiscard]] virtual std::unique_ptr<ITracedProcess>
        createTracedProcess(const std::shared_ptr<const VmiCore::ActiveProcessInformation>& activeProcessInformation,
                            const TracingProfile& tracingProfile) const = 0;

      protected:
        ITracedProcessFactory() = default;
    };

    class TracedProcessFactory : public ITracedProcessFactory
    {
      public:
        TracedProcessFactory(VmiCore::Plugin::PluginInterface* pluginInterface,
                             std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                             std::shared_ptr<ILibrary> library);

        [[nodiscard]] std::unique_ptr<ITracedProcess>
        createTracedProcess(const std::shared_ptr<const VmiCore::ActiveProcessInformation>& activeProcessInformation,
                            const TracingProfile& tracingProfile) const override;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::shared_ptr<IFunctionDefinitions> functionDefinitions;
        std::shared_ptr<ILibrary> library;
    };

}
#endif // APITRACING_TRACEDPROCESSFACTORY_H
