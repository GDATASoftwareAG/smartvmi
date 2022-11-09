#ifndef VMICORE_PLUGININIT_H
#define VMICORE_PLUGININIT_H

#include "PluginInterface.h"

#define VMI_PLUGIN_API_VERSION_INFO                                                                                    \
    [[maybe_unused]] const uint8_t VmiCore::Plugin::API_VERSION = VmiCore::Plugin::PluginInterface::API_VERSION;

namespace VmiCore::Plugin
{
    extern const uint8_t API_VERSION;

    extern "C" bool
    init(PluginInterface* pluginInterface, std::shared_ptr<IPluginConfig> config, std::vector<std::string> args);
}

#endif // VMICORE_PLUGININIT_H
