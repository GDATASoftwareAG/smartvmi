#ifndef VMICORE_CONFIGYAMLPARSER_H
#define VMICORE_CONFIGYAMLPARSER_H

#include "IConfigParser.h"
#include <yaml-cpp/yaml.h>

namespace VmiCore
{
    class ConfigYAMLParser : public IConfigParser
    {
      public:
        ~ConfigYAMLParser() override = default;

        void extractConfiguration(const std::filesystem::path& configurationPath) override;

        void setLogLevel(const std::string& logLevel) override;

        [[nodiscard]] std::string getLogLevel() const override;

        [[nodiscard]] std::filesystem::path getResultsDirectory() const override;

        void setResultsDirectory(const std::filesystem::path& resultsDirectory) override;

        [[nodiscard]] std::string getVmName() const override;

        void setVmName(const std::string& vmName) override;

        [[nodiscard]] std::filesystem::path getSocketPath() override;

        void setSocketPath(const std::filesystem::path& socketPath) override;

        [[nodiscard]] std::string getOffsetsFile() const override;

        [[nodiscard]] std::filesystem::path getPluginDirectory() const override;

        [[nodiscard]] const std::map<const std::string, const std::shared_ptr<Plugin::IPluginConfig>>&
        getPlugins() const override;

      private:
        using vmiConfiguration = struct configuration_t
        {
            std::filesystem::path resultsDirectory;
            std::string logLevel;
            std::string vmName;
            std::filesystem::path socketPath;
            std::string offsetsFile;
            std::filesystem::path pluginDirectory;
            std::map<const std::string, const std::shared_ptr<Plugin::IPluginConfig>> plugins{};
        };
        vmiConfiguration configuration;
        YAML::Node configRootNode;
    };
}

#endif // VMICORE_CONFIGYAMLPARSER_H
