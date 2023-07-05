#include "Config.h"
#include "TracingDefinitions.h"
#include <vmicore/plugins/IPluginConfig.h>

namespace ApiTracing
{
    Config::Config(const VmiCore::Plugin::IPluginConfig& pluginConfig)
    {
        auto configRootNode = pluginConfig.rootNode();
        if (auto configFilePath = pluginConfig.configFilePath())
        {
            configFileDir = configFilePath.value().parent_path();
            configRootNode = YAML::LoadFile(configFilePath.value());
        }
        else
        {
            configFileDir = pluginConfig.mainConfigFileLocation();
        }

        parseFunctionDefinitionsPath(configRootNode);
        parseProfiles(configRootNode);
        parseTracingTargets(configRootNode);
    }

    TracingProfile Config::parseProfile(const YAML::Node& profileNode, const std::string& name)
    {
        TracingProfile profile{.name = name,
                               .traceChilds = profileNode["trace_children"].as<bool>(),
                               .modules = std::vector<ModuleInformation>()};

        for (const auto& tracedModule : profileNode["traced_modules"])
        {
            ModuleInformation moduleInformation{.name = tracedModule.first.as<std::string>(),
                                                .functions = std::vector<std::string>()};

            for (const auto& function : tracedModule.second)
            {
                moduleInformation.functions.push_back(function.as<std::string>());
            }
            profile.modules.push_back(moduleInformation);
        }

        return profile;
    }

    void Config::parseProfiles(const YAML::Node& rootNode)
    {
        for (const auto& profileNode : rootNode["profiles"])
        {
            auto profile = parseProfile(profileNode.second, profileNode.first.as<std::string>());
            profiles[profile.name] = profile;
        }
    }

    void Config::parseTracingTargets(const YAML::Node& rootNode)
    {

        for (const auto& process : rootNode["traced_processes"])
        {
            if (process.second["profile"])
            {
                processTracingProfiles.emplace(process.first.as<std::string>(),
                                               profiles[process.second["profile"].as<std::string>()]);
            }
            else
            {
                processTracingProfiles.emplace(process.first.as<std::string>(), profiles["default"]);
            }
        }
    }

    void Config::parseFunctionDefinitionsPath(const YAML::Node& rootNode)
    {
        functionDefinitions = std::filesystem::path(rootNode["function_definitions"].as<std::string>());
        if (functionDefinitions.is_relative())
        {
            functionDefinitions = configFileDir / functionDefinitions;
        }
    }

    std::optional<TracingProfile> Config::getTracingProfile(std::string_view processName) const
    {
        auto tracingProfile = processTracingProfiles.find(processName);
        if (tracingProfile == processTracingProfiles.end())
        {
            return std::nullopt;
        }

        return tracingProfile->second;
    }

    std::filesystem::path Config::getFunctionDefinitionsPath() const
    {
        return functionDefinitions;
    }

    void Config::addTracingTarget(const std::string& name)
    {
        processTracingProfiles.emplace(name, profiles["default"]);
    }

    void Config::setFunctionDefinitionsPath(const std::filesystem::path& path)
    {
        functionDefinitions = path;
    }
}
