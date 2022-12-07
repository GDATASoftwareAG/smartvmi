#include "TracingTargetsParser.h"
#include "TracingDefinitions.h"

namespace ApiTracing
{
    std::shared_ptr<std::vector<ProcessInformation>>
    TracingTargetsParser::getTracingTargets(const std::filesystem::path& tracingTargetsPath)
    {
        configRootNode = YAML::LoadFile(tracingTargetsPath);
        auto tracingInformation = std::make_shared<std::vector<ProcessInformation>>();
        for (const auto& process : configRootNode["traced_processes"])
        {
            ProcessInformation processInformation{};
            processInformation.traceChilds = process.second["trace_children"].as<bool>();
            processInformation.name = process.first.as<std::string>();

            for (const auto& tracedModule : process.second["traced_modules"])
            {
                ModuleInformation moduleInformation{.name = tracedModule.first.as<std::string>(),
                                                    .functions = std::vector<std::string>()};

                for (const auto& function : tracedModule.second)
                {
                    moduleInformation.functions.emplace_back(function.as<std::string>());
                }
                processInformation.modules.emplace_back(moduleInformation);
            }
            tracingInformation->emplace_back(processInformation);
        }
        return tracingInformation;
    }
}
