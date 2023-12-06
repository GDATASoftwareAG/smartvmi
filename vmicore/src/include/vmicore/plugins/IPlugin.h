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
    /**
     * Will be verified by VMICore in order to ensure ABI compatibility. Has to be an exact match.
     */
    extern const uint8_t API_VERSION;

    /**
     * An abstract class that has to be implemented by the main plugin class.
     */
    class IPlugin
    {
      public:
        virtual ~IPlugin() = default;

        /**
         * Called right before shutting down the application. Is allowed to throw.
         */
        virtual void unload() = 0;

      protected:
        IPlugin() = default;
    };

    /**
     * Entry point for plugin initialization. Will be called by the plugin host (VMICore).
     *
     * @param pluginInterface An object containing all API functions that are exposed to plugins.
     * @param config The plugin specific configuration. See <a href=./IPluginConfig.h>IPluginConfig.h</a href> for
     * details.
     * @param args Commandline arguments passed to this specific plugin. Elements are whitespace separated.
     * @return An instance of the plugin. Has to implement the IPlugin interface. Lifetime will be managed by VMICore.
     */
    extern std::unique_ptr<IPlugin>
    init(PluginInterface* pluginInterface, std::shared_ptr<IPluginConfig> config, std::vector<std::string> args);
}

#endif // APITRACING_IPLUGIN_H
