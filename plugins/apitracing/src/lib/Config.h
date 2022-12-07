#ifndef APITRACING_CONFIG_H
#define APITRACING_CONFIG_H

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vmicore/plugins/PluginInterface.h>

namespace ApiTracing
{
    class ConfigException : public std::runtime_error
    {
      public:
        explicit ConfigException(const std::string& Message) : std::runtime_error(Message.c_str()){};
    };

    class IConfig
    {
      public:
        virtual ~IConfig() = default;

        [[nodiscard]] virtual std::filesystem::path getTracingTargetsPath() const = 0;

      protected:
        IConfig() = default;
    };

    class Config : public IConfig
    {
      public:
        explicit Config(std::filesystem::path tracingTargetsPath);

        ~Config() override = default;

        [[nodiscard]] std::filesystem::path getTracingTargetsPath() const override;

      private:
        std::filesystem::path tracingTargetsPath;
        std::string initialProcessName;
    };
}
#endif // APITRACING_CONFIG_H
