#ifndef TEMPLATE_TEMPLATE_H
#define TEMPLATE_TEMPLATE_H

#include "TemplateCode.h"
#include <vmicore/plugins/IPlugin.h>
#include <vmicore/plugins/PluginInterface.h>

namespace Template
{
    class Template : public VmiCore::Plugin::IPlugin
    {
      public:
        Template(VmiCore::Plugin::PluginInterface* pluginInterface,
                 const VmiCore::Plugin::IPluginConfig& config,
                 std::vector<std::string>& args);

        ~Template() override = default;

        void unload() override{};

      private:
        std::unique_ptr<VmiCore::ILogger> logger;
        std::unique_ptr<TemplateCode> templateCode;
    };
}

#endif // TEMPLATE_TEMPLATE_H
