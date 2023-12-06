#ifndef VMICORE_IPLUGINCONFIG_H
#define VMICORE_IPLUGINCONFIG_H

#include <filesystem>
#include <optional>
#include <string>

#ifdef YAML_CPP_SUPPORT
#include <yaml-cpp/yaml.h>
#else
#include <stdexcept>
#endif

namespace VmiCore::Plugin
{
    /**
     * Defines methods for retrieving plugin specific configuration options. See the
     * <a href=../../../../../plugins/Readme.md>plugin readme</a> for details on how to pass configuration options to a
     * plugin.
     */
    class IPluginConfig
    {
      public:
        virtual ~IPluginConfig() = default;

        /**
         * Returns plugin config as a string. Useful if the config should be parsed by the plugin separately.
         */
        [[nodiscard]] virtual std::string asString() const = 0;

#ifdef YAML_CPP_SUPPORT
        /**
         * Retrieve the plugin config as a yaml-cpp node. VMICore and the plugin should have the same yaml-cpp version,
         * otherwise the behavior is undefined. Currently, there is no way to verify matching versions at run time.
         *
         * @return A reference to a yaml node that represents the root of the plugin config.
         */
        [[nodiscard]] virtual const YAML::Node& rootNode() const = 0;
#else
        // Keep vtable ordinals consistent
        virtual void dummy()
        {
            throw std::runtime_error("This function should never be called!");
        }
#endif

        /**
         * Returns the path of the main config file used by VMICore.
         */
        [[nodiscard]] virtual std::filesystem::path mainConfigFileLocation() const = 0;

        /**
         * If the plugin has a separate config file instead of an inline config, the path to it can be obtained via
         * this function.
         *
         * @return An optional file path. The key "config_file" has to defined in the VMICore config file for the
         * specific plugin. Will return std::nullopt otherwise. If the path is absolute it will be returned as is, if it
         * is relative it will be interpreted relative to the location of the main plugin file.
         */
        [[nodiscard]] virtual std::optional<std::filesystem::path> configFilePath() const = 0;

      protected:
        IPluginConfig() = default;
    };
}

#endif // VMICORE_IPLUGINCONFIG_H
