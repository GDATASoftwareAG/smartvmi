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

    struct ProcessInformation
    {
        bool traceChilds;
        std::string name;
        std::vector<ModuleInformation> modules;

        bool operator==(const ProcessInformation& rhs) const = default;
    };
}
#endif // APITRACING_TRACINGDEFINITIONS_H
