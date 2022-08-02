#pragma once

#include "../src/Config.h"
#include <gmock/gmock.h>

class MockConfig : public IConfig
{
  public:
    MOCK_METHOD(void, parseConfiguration, (const Plugin::IPluginConfig&), (override));
    MOCK_METHOD(std::filesystem::path, getOutputPath, (), (const, override));
    MOCK_METHOD((std::map<std::string, std::vector<std::string>>), getHookTargets, (const std::string&), (const, override));
    MOCK_METHOD(std::string, getInitialProcessName, (), (const, override));
};