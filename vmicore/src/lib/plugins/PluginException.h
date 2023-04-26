#ifndef VMICORE_PLUGINEXCEPTION_H
#define VMICORE_PLUGINEXCEPTION_H

#include <stdexcept>
#include <string>
#include <string_view>

namespace VmiCore
{
    class PluginException : public std::runtime_error
    {
      public:
        PluginException(std::string pluginName, const std::string& message)
            : runtime_error(message), pluginName(std::move(pluginName))
        {
        }

        inline std::string_view plugin() const
        {
            return pluginName;
        }

      private:
        std::string pluginName;
    };
}

#endif // VMICORE_PLUGINEXCEPTION_H
