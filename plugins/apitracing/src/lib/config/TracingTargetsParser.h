#ifndef APITRACING_TRACINGTARGETSPARSER_H
#define APITRACING_TRACINGTARGETSPARSER_H

#include "TracingDefinitions.h"
#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <yaml-cpp/yaml.h>

namespace ApiTracing
{
    class ITracingTargetsParser
    {
      public:
        virtual ~ITracingTargetsParser() = default;

        [[nodiscard]] virtual std::optional<TracingProfile> getTracingProfile(std::string_view processName) const = 0;

        virtual void addTracingTarget(const std::string& name) = 0;

      protected:
        ITracingTargetsParser() = default;
    };

    class TracingTargetsParser : public ITracingTargetsParser
    {
      public:
        explicit TracingTargetsParser(const std::filesystem::path& tracingTargetsPath);

        ~TracingTargetsParser() override = default;

        [[nodiscard]] std::optional<TracingProfile> getTracingProfile(std::string_view processName) const override;

        void addTracingTarget(const std::string& name) override;

      private:
        YAML::Node configRootNode;
        std::map<std::string, TracingProfile, std::less<>> profiles;
        std::map<std::string, TracingProfile, std::less<>> processTracingProfiles;

        TracingProfile parseProfile(const YAML::Node& profileNode, const std::string& name) const;

        void parseProfiles();

        void parseTracingTargets();
    };
}
#endif // APITRACING_TRACINGTARGETSPARSER_H
