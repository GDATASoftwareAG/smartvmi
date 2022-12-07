#ifndef APITRACING_TRACINGTARGETSPARSER_H
#define APITRACING_TRACINGTARGETSPARSER_H

#include "TracingDefinitions.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace ApiTracing
{
    class ITracingTargetsParser
    {
      public:
        virtual ~ITracingTargetsParser() = default;

        [[nodiscard]] virtual std::shared_ptr<std::vector<ProcessInformation>>
        getTracingTargets(const std::filesystem::path& tracingTargetsPath) = 0;

      protected:
        ITracingTargetsParser() = default;
    };

    class TracingTargetsParser : public ITracingTargetsParser
    {
      public:
        ~TracingTargetsParser() override = default;

        [[nodiscard]] std::shared_ptr<std::vector<ProcessInformation>>
        getTracingTargets(const std::filesystem::path& tracingTargetsPath) override;

      private:
        using vmiConfiguration = struct configuration_t
        {
            std::filesystem::path resultsDirectory;
        };
        vmiConfiguration configuration;
        YAML::Node configRootNode;
    };
}
#endif // APITRACING_TRACINGTARGETSPARSER_H
