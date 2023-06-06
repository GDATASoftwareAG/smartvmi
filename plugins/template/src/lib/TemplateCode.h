#ifndef TEMPLATE_TEMPLATECODE_H
#define TEMPLATE_TEMPLATECODE_H

#include <vmicore/os/ActiveProcessInformation.h>
#include <vmicore/plugins/PluginInterface.h>

namespace Template
{
    class TemplateCode
    {
      public:
        TemplateCode(VmiCore::Plugin::PluginInterface* pluginInterface,
                     std::shared_ptr<VmiCore::IIntrospectionAPI> lowLevelIntrospectionApi);

        void doStuffWithProcessStart(std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation);

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::shared_ptr<VmiCore::IIntrospectionAPI> lowLevelIntrospectionApi;
    };
}

#endif // TEMPLATE_TEMPLATECODE_H
