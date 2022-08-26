#ifndef VMICORE_IPLUGINCONFIG_H
#define VMICORE_IPLUGINCONFIG_H

#include <optional>
#include <string>
#include <vector>

namespace VmiCore::Plugin
{
    class IPluginConfig
    {
      public:
        virtual ~IPluginConfig() = default;

        [[nodiscard]] virtual std::optional<std::string> getString(const std::string& element) const = 0;

        virtual void overrideString(const std::string& element, const std::string& value) = 0;

        [[nodiscard]] virtual std::optional<std::vector<std::string>>
        getStringSequence(const std::string& element) const = 0;

      protected:
        IPluginConfig() = default;
    };
} // namespace Plugin

#endif // VMICORE_IPLUGINCONFIG_H
