#include "ConfigYAMLParser.h"
#include "PluginConfig.h"
#include <iostream>

namespace VmiCore
{
    void ConfigYAMLParser::extractConfiguration(const std::filesystem::path& configurationPath)
    {
        configRootNode = YAML::LoadFile(configurationPath);
        configuration.resultsDirectory = configRootNode["results_directory"].as<std::string>();
        configuration.logLevel = configRootNode["log_level"].as<std::string>();
        configuration.vmName = configRootNode["vm"]["name"].as<std::string>();
        if (configRootNode["vm"]["socket"].IsDefined())
        {
            configuration.socketPath = configRootNode["vm"]["socket"].as<std::string>();
        }
        configuration.offsetsFile = configRootNode["vm"]["offsets_file"].as<std::string>();
        configuration.pluginDirectory = configRootNode["plugin_system"]["directory"].as<std::string>();

        for (const auto& node : configRootNode["plugin_system"]["plugins"])
        {
            configuration.plugins.insert(
                std::make_pair(node.first.as<std::string>(), std::make_shared<PluginConfig>(node.second)));
        }
    }

    std::filesystem::path ConfigYAMLParser::getResultsDirectory() const
    {
        return configuration.resultsDirectory;
    }

    void ConfigYAMLParser::setLogLevel(const std::string& logLevel)
    {
        configuration.logLevel = logLevel;
    }

    std::string ConfigYAMLParser::getLogLevel() const
    {
        return configuration.logLevel;
    }

    void ConfigYAMLParser::setResultsDirectory(const std::filesystem::path& resultsDirectory)
    {
        configuration.resultsDirectory = resultsDirectory;
    }

    std::string ConfigYAMLParser::getVmName() const
    {
        return configuration.vmName;
    }

    void ConfigYAMLParser::setVmName(const std::string& vmName)
    {
        configuration.vmName = vmName;
    }

    std::filesystem::path ConfigYAMLParser::getSocketPath()
    {
        return configuration.socketPath;
    }

    void ConfigYAMLParser::setSocketPath(const std::filesystem::path& socketPath)
    {
        configuration.socketPath = socketPath;
    }

    std::string ConfigYAMLParser::getOffsetsFile() const
    {
        return configuration.offsetsFile;
    }

    std::filesystem::path ConfigYAMLParser::getPluginDirectory() const
    {
        return configuration.pluginDirectory;
    }

    const std::map<const std::string, const std::shared_ptr<Plugin::IPluginConfig>>&
    ConfigYAMLParser::getPlugins() const
    {
        return configuration.plugins;
    }
}
