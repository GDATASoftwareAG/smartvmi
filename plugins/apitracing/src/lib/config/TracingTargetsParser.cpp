#include "TracingTargetsParser.h"
#include "TracingDefinitions.h"

namespace ApiTracing
{
    TracingTargetsParser::TracingTargetsParser(const std::filesystem::path& tracingTargetsPath)
        : configRootNode(YAML::LoadFile(tracingTargetsPath))
    {
        parseProfiles();
        parseTracingTargets();
    }

    TracingProfile TracingTargetsParser::parseProfile(const YAML::Node& profileNode, const std::string& name) const
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

    void TracingTargetsParser::parseProfiles()
    {
        for (const auto& profileNode : configRootNode["profiles"])
        {
            auto profile = parseProfile(profileNode.second, profileNode.first.as<std::string>());
            profiles[profile.name] = profile;
        }
    }

    void TracingTargetsParser::parseTracingTargets()
    {

        for (const auto& process : configRootNode["traced_processes"])
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

    std::optional<TracingProfile> TracingTargetsParser::getTracingProfile(std::string_view processName) const
    {
        auto tracingProfile = processTracingProfiles.find(processName);
        if (tracingProfile == processTracingProfiles.end())
        {
            return std::nullopt;
        }

        return tracingProfile->second;
    }

    void TracingTargetsParser::addTracingTarget(const std::string& name)
    {
        processTracingProfiles.emplace(name, profiles["default"]);
    }
}
