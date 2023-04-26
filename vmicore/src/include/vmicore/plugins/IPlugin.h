#ifndef APITRACING_IPLUGIN_H
#define APITRACING_IPLUGIN_H

#include "IPluginConfig.h"
#include "PluginInterface.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define VMI_PLUGIN_API_VERSION_INFO                                                                                    \
    [[maybe_unused]] const uint8_t VmiCore::Plugin::API_VERSION = VmiCore::Plugin::PluginInterface::API_VERSION;

namespace VmiCore::Plugin
{
    // Will be verified by VMICore in order to ensure ABI compatibility.
    extern const uint8_t API_VERSION;

    class IPlugin
    {
      public:
        virtual ~IPlugin() = default;

        // Called right before shutting down the application. Is allowed to throw.
        virtual void unload() = 0;

      protected:
        IPlugin() = default;
    };

    extern std::unique_ptr<IPlugin>
    init(PluginInterface* pluginInterface, std::shared_ptr<IPluginConfig> config, std::vector<std::string> args);
}

#endif // APITRACING_IPLUGIN_H
