#include "TemplateCode.h"
#include "Filenames.h"
#include <utility>

namespace Template
{
    using VmiCore::ActiveProcessInformation;
    using VmiCore::Plugin::PluginInterface;

    TemplateCode::TemplateCode(VmiCore::Plugin::PluginInterface* pluginInterface,
                               std::shared_ptr<VmiCore::IIntrospectionAPI> lowLevelIntrospectionApi)
        : pluginInterface(pluginInterface),
          logger(this->pluginInterface->newNamedLogger(TEMPLATE_LOGGER_NAME)),
          lowLevelIntrospectionApi(std::move(lowLevelIntrospectionApi))
    {
    }

    void TemplateCode::doStuffWithProcessStart(const ActiveProcessInformation& processInformation)
    {
        auto dtbContent = lowLevelIntrospectionApi->read64PA(processInformation.processCR3);
        // Example for structured logging calls
        logger->info("Process start triggered.",
                     {{"processDtb", processInformation.processCR3},
                      {"processId", processInformation.pid},
                      {"dtbContent", dtbContent}});
    }
}