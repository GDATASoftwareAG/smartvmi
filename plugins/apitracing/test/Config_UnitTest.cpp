#include <config/Config.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>
#include <vmicore_test/plugins/mock_PluginConfig.h>

using testing::Return;
using VmiCore::Plugin::MockPluginConfig;

namespace ApiTracing
{
    constexpr std::string_view defaultMainConfigFileDir = "/usr/local/var/";

    std::unique_ptr<MockPluginConfig> createMockPluginConfig(const YAML::Node& configNode)
    {
        auto mockPluginConfig = std::make_unique<MockPluginConfig>();
        ON_CALL(*mockPluginConfig, rootNode())
            .WillByDefault([configNode]() -> const YAML::Node& { return configNode; });
        ON_CALL(*mockPluginConfig, configFilePath()).WillByDefault(Return(std::nullopt));
        ON_CALL(*mockPluginConfig, mainConfigFileLocation()).WillByDefault(Return(defaultMainConfigFileDir));

        return mockPluginConfig;
    }

    std::unique_ptr<MockPluginConfig> createMockPluginConfig(std::string_view configurationFile)
    {
        auto mockPluginConfig = std::make_unique<MockPluginConfig>();
        ON_CALL(*mockPluginConfig, rootNode())
            .WillByDefault([defaultNode = YAML::Node{}]() -> const YAML::Node& { return defaultNode; });
        ON_CALL(*mockPluginConfig, configFilePath()).WillByDefault(Return(configurationFile));

        return mockPluginConfig;
    }

    TEST(ConfigTests, constructor_functionDefinitionsKeyMissing_throws)
    {
        YAML::Node emptyConfigRootNode;

        EXPECT_ANY_THROW(std::make_unique<Config>(*createMockPluginConfig(emptyConfigRootNode)));
    }

    TEST(ConfigTests, getTracingTargets_calcProcess_correctProfile)
    {

        auto config = std::make_unique<Config>(*createMockPluginConfig("testConfiguration.yaml"));
        TracingProfile expectedTracingProfile{
            .name = "calc",
            .traceChilds = true,
            .modules = {{.name = "ntdll.dll", .functions = {{"function1"}, {"function2"}}},
                        {.name = "kernel32.dll", .functions = {{"kernelfunction1"}, {"kernelfunction2"}}}}};

        auto tracingProfile = config->getTracingProfile("calc.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST(ConfigTests, getTracingTargets_processWithoutProfile_defaultProfile)
    {
        auto tracingTargetsParser = std::make_unique<Config>(*createMockPluginConfig("testConfiguration.yaml"));
        TracingProfile expectedTracingProfile{
            .name = "default", .traceChilds = true, .modules = {{.name = "ntdll.dll", .functions = {{"function1"}}}}};

        auto tracingProfile = tracingTargetsParser->getTracingProfile("notepad.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST(ConfigTests, getTracingTargets_unknownProcessName_nullopt)
    {
        auto tracingTargetsParser = std::make_unique<Config>(*createMockPluginConfig("testConfiguration.yaml"));

        auto tracingProfile = tracingTargetsParser->getTracingProfile("unknown.exe");

        EXPECT_FALSE(tracingProfile);
    }

    TEST(ConfigTests, getFunctionDefinitionsPath_configFromVmiCoreConfigFile_correctAbsolutePath)
    {
        YAML::Node configRootNode;
        configRootNode["function_definitions"] = "test.yaml";

        auto config = std::make_unique<Config>(*createMockPluginConfig(configRootNode));

        EXPECT_EQ(config->getFunctionDefinitionsPath(), std::filesystem::path(defaultMainConfigFileDir) / "test.yaml");
    }

    TEST(ConfigTests, getFunctionDefinitionsPath_configWithFunctionDefinitionsKey_correctFilePath)
    {
        auto config = std::make_unique<Config>(*createMockPluginConfig("testConfiguration.yaml"));

        EXPECT_NO_THROW(YAML::LoadFile(config->getFunctionDefinitionsPath()));
    }
}
