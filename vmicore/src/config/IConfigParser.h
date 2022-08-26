#ifndef VMICORE_CONFIGPARSER_H
#define VMICORE_CONFIGPARSER_H

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vmicore/plugins/IPluginConfig.h>

namespace VmiCore
{
    class IConfigParser
    {
      public:
        virtual ~IConfigParser() = default;

        virtual void extractConfiguration(const std::filesystem::path& configurationPath) = 0;

        [[nodiscard]] virtual std::filesystem::path getResultsDirectory() const = 0;

        virtual void setResultsDirectory(const std::filesystem::path& resultsDirectory) = 0;

        virtual void setLogLevel(const std::string& logLevel) = 0;

        [[nodiscard]] virtual std::string getLogLevel() const = 0;

        [[nodiscard]] virtual std::string getVmName() const = 0;

        virtual void setVmName(const std::string& vmName) = 0;

        [[nodiscard]] virtual std::filesystem::path getSocketPath() = 0;

        virtual void setSocketPath(const std::filesystem::path& socketPath) = 0;

        [[nodiscard]] virtual std::string getOffsetsFile() const = 0;

        [[nodiscard]] virtual std::filesystem::path getPluginDirectory() const = 0;

        [[nodiscard]] virtual const std::map<const std::string, const std::shared_ptr<Plugin::IPluginConfig>>&
        getPlugins() const = 0;

      protected:
        IConfigParser() = default;
    };
}

#endif // VMICORE_CONFIGPARSER_H
