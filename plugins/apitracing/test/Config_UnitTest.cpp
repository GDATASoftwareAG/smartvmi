#include <config/Config.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/plugins/mock_PluginConfig.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>

using testing::_; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
using testing::NiceMock;
using testing::Return;
using VmiCore::Plugin::MockPluginConfig;
using VmiCore::Plugin::MockPluginInterface;

namespace ApiTracing
{
    constexpr std::string_view defaultMainConfigFileDir = "/usr/local/var/";

    class ConfigTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<NiceMock<MockPluginInterface>>();

        void SetUp() override
        {
            ON_CALL(*pluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });
        }
    };

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

    TEST_F(ConfigTestFixture, constructor_functionDefinitionsKeyMissing_throws)
    {
        YAML::Node emptyConfigRootNode;

        EXPECT_ANY_THROW(std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig(emptyConfigRootNode)));
    }

    TEST_F(ConfigTestFixture, getTracingProfile_calcProcess_correctProfile)
    {
        auto config =
            std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig("testConfiguration.yaml"));
        TracingProfile expectedTracingProfile{
            .name = "calc",
            .traceChildren = true,
            .modules = {{.name = "ntdll.dll", .functions = {{"function1"}, {"function2"}}},
                        {.name = "kernel32.dll", .functions = {{"kernelfunction1"}, {"kernelfunction2"}}}}};

        auto tracingProfile = config->getTracingProfile("calc.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST_F(ConfigTestFixture, getTracingProfile_processWithoutProfile_defaultProfile)
    {
        auto config =
            std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig("testConfiguration.yaml"));
        TracingProfile expectedTracingProfile{
            .name = "default", .traceChildren = true, .modules = {{.name = "ntdll.dll", .functions = {{"function1"}}}}};

        auto tracingProfile = config->getTracingProfile("notepad.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST_F(ConfigTestFixture, getTracingProfile_unknownProcessName_nullopt)
    {
        auto config =
            std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig("testConfiguration.yaml"));

        auto tracingProfile = config->getTracingProfile("unknown.exe");

        EXPECT_FALSE(tracingProfile);
    }

    TEST_F(ConfigTestFixture, getFunctionDefinitionsPath_configFromVmiCoreConfigFile_correctAbsolutePath)
    {
        YAML::Node configRootNode;
        configRootNode["function_definitions"] = "test.yaml";

        auto config = std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig(configRootNode));

        EXPECT_EQ(config->getFunctionDefinitionsPath(), std::filesystem::path(defaultMainConfigFileDir) / "test.yaml");
    }

    TEST_F(ConfigTestFixture, getFunctionDefinitionsPath_configWithFunctionDefinitionsKey_correctFilePath)
    {
        auto config =
            std::make_unique<Config>(pluginInterface.get(), *createMockPluginConfig("testConfiguration.yaml"));

        EXPECT_NO_THROW(YAML::LoadFile(config->getFunctionDefinitionsPath()));
    }
}
