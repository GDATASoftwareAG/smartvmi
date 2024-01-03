#include "TemplateCode.h"
#include "Filenames.h"
#include <utility>
#include <vmicore/callback.h>

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
        // Register required events
        pluginInterface->registerProcessStartEvent(VMICORE_SETUP_MEMBER_CALLBACK(doStuffWithProcessStart));
    }

    // As an example when a new process is started this callback is called. Check the PluginInterface.h for additional
    // available events.
    void TemplateCode::doStuffWithProcessStart(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        auto dtbContent = lowLevelIntrospectionApi->read64PA(processInformation->processUserDtb);
        // Example for structured logging calls
        logger->info("Process start triggered.",
                     {{"processDtb", processInformation->processUserDtb},
                      {"processId", processInformation->pid},
                      {"dtbContent", dtbContent}});
    }
}
