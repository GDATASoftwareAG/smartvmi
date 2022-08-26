#include "PluginConfig.h"

namespace VmiCore
{
    PluginConfig::PluginConfig(const YAML::Node& pluginNode) : pluginRootNode(pluginNode) {}

    std::optional<std::string> PluginConfig::getString(const std::string& element) const
    {
        try
        {
            return pluginRootNode[element].as<std::string>();
        }
        catch (const YAML::Exception&)
        {
            return std::nullopt;
        }
    }

    void PluginConfig::overrideString(const std::string& element, const std::string& value)
    {
        pluginRootNode[element] = value;
    }

    std::optional<std::vector<std::string>> PluginConfig::getStringSequence(const std::string& element) const
    {
        try
        {
            return pluginRootNode[element].as<std::vector<std::string>>();
        }
        catch (const YAML::Exception&)
        {
            return std::nullopt;
        }
    }
}
