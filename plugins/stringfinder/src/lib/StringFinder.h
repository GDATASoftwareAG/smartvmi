#ifndef STRINGFINDER_STRINGFINDER_H
#define STRINGFINDER_STRINGFINDER_H

#include <vmicore/plugins/IPlugin.h>
#include <vmicore/plugins/PluginInterface.h>

namespace StringFinder
{
    class StringFinder : public VmiCore::Plugin::IPlugin
    {
      public:
        StringFinder(VmiCore::Plugin::PluginInterface* pluginInterface,
                 const VmiCore::Plugin::IPluginConfig& config,
                 std::vector<std::string>& args);

        ~StringFinder() override = default;

        void findInMemoryString(std::shared_ptr<const VmiCore::ActiveProcessInformation> processInfo);

        void unload() override;

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::u16string searchString;
    };
}

#endif // STRINGFINDER_STRINGFINDER_H
