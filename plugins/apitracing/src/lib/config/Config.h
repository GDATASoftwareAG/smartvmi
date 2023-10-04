#ifndef APITRACING_CONFIG_H
#define APITRACING_CONFIG_H

#include "TracingDefinitions.h"
#include <filesystem>
#include <map>
#include <optional>
#include <string_view>
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/IPluginConfig.h>
#include <vmicore/plugins/PluginInterface.h>
#include <yaml-cpp/yaml.h>

namespace ApiTracing
{
    class IConfig
    {
      public:
        virtual ~IConfig() = default;

        [[nodiscard]] virtual std::optional<TracingProfile> getTracingProfile(std::string_view processName) const = 0;

        [[nodiscard]] virtual std::filesystem::path getFunctionDefinitionsPath() const = 0;

        virtual void addTracingTarget(const std::string& name) = 0;

        virtual void setFunctionDefinitionsPath(const std::filesystem::path& functionDefinitions) = 0;

      protected:
        IConfig() = default;
    };

    class Config : public IConfig
    {
      public:
        explicit Config(const VmiCore::Plugin::PluginInterface* pluginInterface,
                        const VmiCore::Plugin::IPluginConfig& pluginConfig);

        ~Config() override = default;

        [[nodiscard]] std::optional<TracingProfile> getTracingProfile(std::string_view processName) const override;

        [[nodiscard]] std::filesystem::path getFunctionDefinitionsPath() const override;

        void addTracingTarget(const std::string& name) override;

        void setFunctionDefinitionsPath(const std::filesystem::path& path) override;

      private:
        std::unique_ptr<VmiCore::ILogger> logger;
        std::filesystem::path configFileDir;
        std::filesystem::path functionDefinitions;
        std::map<std::string, TracingProfile, std::less<>> profiles;
        std::map<std::string, TracingProfile, std::less<>> processTracingProfiles;

        [[nodiscard]] static TracingProfile parseProfile(const YAML::Node& profileNode, const std::string& name);

        void parseProfiles(const YAML::Node& rootNode);

        void parseTracingTargets(const YAML::Node& rootNode);

        void parseFunctionDefinitionsPath(const YAML::Node& rootNode);
    };
}
#endif // APITRACING_CONFIG_H
