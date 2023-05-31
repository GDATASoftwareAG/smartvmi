#ifndef INMEMORYSCANNER_INMEMORY_H
#define INMEMORYSCANNER_INMEMORY_H

#include "Scanner.h"
#include <memory>
#include <vmicore/plugins/IPlugin.h>
#include <vmicore/plugins/PluginInterface.h>

namespace InMemoryScanner
{
    class InMemory : public VmiCore::Plugin::IPlugin
    {
      public:
        InMemory(VmiCore::Plugin::PluginInterface* pluginInterface,
                 const VmiCore::Plugin::IPluginConfig& config,
                 std::vector<std::string>& args);

        ~InMemory() override = default;

        void unload() override;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::unique_ptr<Scanner> scanner;
    };
}

#endif // INMEMORYSCANNER_INMEMORY_H
