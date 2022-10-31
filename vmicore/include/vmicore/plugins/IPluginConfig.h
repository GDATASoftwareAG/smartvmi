#ifndef VMICORE_IPLUGINCONFIG_H
#define VMICORE_IPLUGINCONFIG_H

#include <filesystem>
#include <optional>
#include <string>

#ifdef YAML_CPP_SUPPORT
#include <yaml-cpp/yaml.h>
#endif

namespace VmiCore::Plugin
{
    class IPluginConfig
    {
      public:
        virtual ~IPluginConfig() = default;

        [[nodiscard]] virtual std::string asString() const = 0;

#ifdef YAML_CPP_SUPPORT
        [[nodiscard]] virtual const YAML::Node& rootNode() const = 0;
#endif

        [[nodiscard]] virtual std::optional<std::filesystem::path> configFilePath() const = 0;

      protected:
        IPluginConfig() = default;
    };
}

#endif // VMICORE_IPLUGINCONFIG_H
