#ifndef VMICORE_PLUGINCONFIG_H
#define VMICORE_PLUGINCONFIG_H

#include <vmicore/plugins/IPluginConfig.h>

namespace VmiCore
{
    class PluginConfig : public Plugin::IPluginConfig
    {
      public:
        PluginConfig(const YAML::Node& pluginNode, std::filesystem::path mainConfigFileDir);

        ~PluginConfig() override = default;

        [[nodiscard]] std::string asString() const override;

        [[nodiscard]] const YAML::Node& rootNode() const override;

        [[nodiscard]] std::optional<std::filesystem::path> configFilePath() const override;

      private:
        YAML::Node pluginRootNode;
        std::filesystem::path mainConfigFileDir;
    };
}

#endif // VMICORE_PLUGINCONFIG_H
