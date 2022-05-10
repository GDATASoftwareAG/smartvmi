#ifndef VMICORE_MOCK_CONFIGINTERFACE_H
#define VMICORE_MOCK_CONFIGINTERFACE_H

#include "../../src/config/IConfigParser.h"
#include <gmock/gmock.h>

class MockConfigInterface : public IConfigParser
{
  public:
    MOCK_METHOD(void, extractConfiguration, (const std::filesystem::path&), (override));

    MOCK_METHOD(std::filesystem::path, getResultsDirectory, (), (const override));

    MOCK_METHOD(void, setResultsDirectory, (const std::filesystem::path&), (override));

    MOCK_METHOD(void, setLogLevel, (const std::string&), (override));

    MOCK_METHOD(std::string, getLogLevel, (), (const override));

    MOCK_METHOD(std::string, getVmName, (), (const override));

    MOCK_METHOD(void, setVmName, (const std::string&), (override));

    MOCK_METHOD(std::filesystem::path, getSocketPath, (), (override));

    MOCK_METHOD(void, setSocketPath, (const std::filesystem::path&), (override));

    MOCK_METHOD(std::string, getOffsetsFile, (), (const override));

    MOCK_METHOD(std::filesystem::path, getPluginDirectory, (), (const override));

    MOCK_METHOD((const std::map<const std::string, const std::shared_ptr<Plugin::IPluginConfig>>&),
                getPlugins,
                (),
                (const override));

    MOCK_METHOD(void, logConfigurationToFile, (), (const override));
};

#endif // VMICORE_MOCK_CONFIGINTERFACE_H
