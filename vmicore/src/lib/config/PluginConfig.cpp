#include "PluginConfig.h"
#include <utility>

namespace VmiCore
{
    PluginConfig::PluginConfig(const YAML::Node& pluginNode, std::filesystem::path mainConfigFileDir)
        : pluginRootNode(Clone(pluginNode)), mainConfigFileDir(std::move(mainConfigFileDir))
    {
    }

    std::string PluginConfig::asString() const
    {
        return Dump(pluginRootNode);
    }

    const YAML::Node& PluginConfig::rootNode() const
    {
        return pluginRootNode;
    }

    std::filesystem::path PluginConfig::mainConfigFileLocation() const
    {
        return mainConfigFileDir;
    }

    std::optional<std::filesystem::path> PluginConfig::configFilePath() const
    {
        try
        {
            auto configFile = std::filesystem::path(pluginRootNode["config_file"].as<std::string>());
            if (configFile.is_absolute())
            {
                return configFile;
            }

            return mainConfigFileDir / configFile;
        }
        catch (const YAML::Exception&)
        {
            return std::nullopt;
        }
    }
}
