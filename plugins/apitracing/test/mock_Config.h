#pragma once

#include "../src/Config.h"
#include <gmock/gmock.h>

class MockConfig : public IConfig
{
  public:
    MOCK_METHOD(void, parseConfiguration, (const Plugin::IPluginConfig&), (override));
    MOCK_METHOD(std::filesystem::path, getOutputPath, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, getHookTargets, (), (const, override));
};