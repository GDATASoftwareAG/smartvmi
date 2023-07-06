#ifndef VMICORE_MOCK_PLUGINCONFIG_H
#define VMICORE_MOCK_PLUGINCONFIG_H

#include <gmock/gmock.h>
#include <vmicore/plugins/IPluginConfig.h>

namespace VmiCore::Plugin
{
    class MockPluginConfig : public IPluginConfig
    {
      public:
        MOCK_METHOD(std::string, asString, (), (const, override));

#ifdef YAML_CPP_SUPPORT
        MOCK_METHOD(const YAML::Node&, rootNode, (), (const, override));
#else
        MOCK_METHOD(void, dummy, (), (override));
#endif

        MOCK_METHOD(std::filesystem::path, mainConfigFileLocation, (), (const, override));

        MOCK_METHOD(std::optional<std::filesystem::path>, configFilePath, (), (const, override));
    };
}

#endif // VMICORE_MOCK_PLUGINCONFIG_H
