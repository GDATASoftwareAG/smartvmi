#include "TracedProcessFactory.h"

#include <utility>

namespace ApiTracing
{
    TracedProcessFactory::TracedProcessFactory(VmiCore::Plugin::PluginInterface* pluginInterface,
                                               std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                                               std::shared_ptr<ILibrary> library)
        : pluginInterface(pluginInterface),
          functionDefinitions(std::move(functionDefinitions)),
          library(std::move(library))
    {
    }

    std::unique_ptr<ITracedProcess> TracedProcessFactory::createTracedProcess(
        const std::shared_ptr<const VmiCore::ActiveProcessInformation>& activeProcessInformation,
        const TracingProfile& tracingProfile) const
    {
        return std::make_unique<TracedProcess>(
            pluginInterface, functionDefinitions, library, activeProcessInformation, tracingProfile);
    }
}
