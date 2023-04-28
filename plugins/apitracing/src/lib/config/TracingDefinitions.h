#ifndef APITRACING_TRACINGDEFINITIONS_H
#define APITRACING_TRACINGDEFINITIONS_H

#include <string>
#include <vector>

namespace ApiTracing
{
    struct ModuleInformation
    {
        std::string name;
        std::vector<std::string> functions;

        bool operator==(const ModuleInformation& rhs) const = default;
    };

    struct TracingProfile
    {
        std::string name;
        bool traceChilds;
        std::vector<ModuleInformation> modules;

        bool operator==(const TracingProfile& rhs) const = default;
    };

    struct ProcessTracingConfig
    {
        std::string name;
        TracingProfile profile;

        bool operator==(const ProcessTracingConfig& rhs) const = default;
    };
}
#endif // APITRACING_TRACINGDEFINITIONS_H
