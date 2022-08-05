#include "../../src/plugins/PluginSystem.h"
#include <gmock/gmock.h>

class MockPluginSystem : public IPluginSystem
{
  public:
    MOCK_METHOD(std::unique_ptr<std::vector<uint8_t>>,
                readProcessMemoryRegion,
                (pid_t, Plugin::virtual_address_t, size_t),
                (const override));

    MOCK_METHOD(std::unique_ptr<std::vector<MemoryRegion>>, getProcessMemoryRegions, (pid_t), (const override));

    MOCK_METHOD(std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>,
                getRunningProcesses,
                (),
                (const override));

    MOCK_METHOD(void, registerProcessTerminationEvent, (Plugin::processTerminationCallback_f), (override));

    MOCK_METHOD(void, registerShutdownEvent, (Plugin::shutdownCallback_f), (override));

    MOCK_METHOD(std::unique_ptr<std::string>, getResultsDir, (), (const override));

    MOCK_METHOD(void, writeToFile, (const std::string&, const std::string&), (const override));

    MOCK_METHOD(void, writeToFile, (const std::string&, const std::vector<uint8_t>&), (const override));

    MOCK_METHOD(void, logMessage, (Plugin::LogLevel, const std::string&, const std::string&), (const override));

    MOCK_METHOD(void, sendErrorEvent, (const std::string&), (const override));

    MOCK_METHOD(void, sendInMemDetectionEvent, (const std::string&), (const override));

    MOCK_METHOD(void,
                initializePlugin,
                (const std::string&, std::shared_ptr<Plugin::IPluginConfig>, const std::vector<std::string>& args),
                (override));

    MOCK_METHOD(void,
                passProcessTerminationEventToRegisteredPlugins,
                (std::shared_ptr<const ActiveProcessInformation>),
                (override));

    MOCK_METHOD(void, passShutdownEventToRegisteredPlugins, (), (override));
};
