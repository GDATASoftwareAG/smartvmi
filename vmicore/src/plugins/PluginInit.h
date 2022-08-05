#ifndef VMICORE_PLUGININIT_H
#define VMICORE_PLUGININIT_H

#include "PluginInterface.h"

namespace Plugin
{
    extern "C" bool
    init(PluginInterface* pluginInterface, std::shared_ptr<IPluginConfig> config, std::vector<std::string> args);
}

#endif // VMICORE_PLUGININIT_H
