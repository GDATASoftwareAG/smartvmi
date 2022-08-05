#pragma once

#include <PluginInterface.h>
#include <gmock/gmock.h>

namespace Plugin
{

    class MockPluginInterface : public PluginInterface
    {
      public:
        MOCK_METHOD(std::unique_ptr<std::vector<uint8_t>>,
                    readProcessMemoryRegion,
                    (pid_t pid, virtual_address_t address, size_t numberOfBytes),
                    (const, override));
        MOCK_METHOD(std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>,
                    getRunningProcesses,
                    (),
                    (const, override));
        MOCK_METHOD(void,
                    registerProcessTerminationEvent,
                    (processTerminationCallback_f terminationCallback),
                    (override));
        MOCK_METHOD(void, registerShutdownEvent, (shutdownCallback_f shutdownCallback), (override));
        MOCK_METHOD(std::unique_ptr<std::string>, getResultsDir, (), (const, override));
        MOCK_METHOD(void, writeToFile, (const std::string& filename, const std::string& message), (const, override));
        MOCK_METHOD(void,
                    writeToFile,
                    (const std::string& filename, const std::vector<uint8_t>& data),
                    (const, override));
        MOCK_METHOD(void,
                    logMessage,
                    (LogLevel logLevel, const std::string& filename, const std::string& message),
                    (const, override));
        MOCK_METHOD(void, sendErrorEvent, (const std::string& message), (const, override));
        MOCK_METHOD(void, sendInMemDetectionEvent, (const std::string& message), (const, override));
    };
}
