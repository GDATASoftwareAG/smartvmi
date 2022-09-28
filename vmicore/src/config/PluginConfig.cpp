#include "PluginConfig.h"

namespace VmiCore
{
    PluginConfig::PluginConfig(const YAML::Node& pluginNode) : pluginRootNode(Clone(pluginNode)) {}

    std::string PluginConfig::asString() const
    {
        return Dump(pluginRootNode);
    }

    const YAML::Node& PluginConfig::rootNode() const
    {
        return pluginRootNode;
    }

    std::optional<std::filesystem::path> PluginConfig::configFilePath() const
    {
        try
        {
            return pluginRootNode["config_file"].as<std::string>();
        }
        catch (const YAML::Exception&)
        {
            return std::nullopt;
        }
    }
}
