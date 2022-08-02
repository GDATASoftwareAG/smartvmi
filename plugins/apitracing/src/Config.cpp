#include "Config.h"
#include "Filenames.h"
#include <algorithm>

Config::Config(const Plugin::PluginInterface* pluginInterface) : pluginInterface(pluginInterface) {}

void Config::parseConfiguration(const Plugin::IPluginConfig& config)
{
    outputPath = config.getString("output_path").value();

    auto hookedFunctions = config.getStringSequence("ntdll").value_or(std::vector<std::string>());
    std::copy(hookedFunctions.begin(), hookedFunctions.end(), std::inserter(hookList, hookList.end()));

    for (auto& element : hookedFunctions)
    {
        pluginInterface->logMessage(
            Plugin::LogLevel::info, LOG_FILENAME, std::string("Got hooked function \"").append(element).append("\""));
    }
}

std::vector<std::string> Config::getHookTargets() const
{
    return hookList;
}

std::filesystem::path Config::getOutputPath() const
{
    return outputPath;
}