#include <gmock/gmock.h>
#include <plugins/PluginSystem.h>

namespace VmiCore
{
    class MockPluginSystem : public IPluginSystem
    {
      public:
        MOCK_METHOD(std::unique_ptr<IMemoryMapping>,
                    mapProcessMemoryRegion,
                    (addr_t, addr_t, std::size_t),
                    (const override));

        MOCK_METHOD(std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>,
                    getRunningProcesses,
                    (),
                    (const override));

        MOCK_METHOD(void,
                    registerProcessStartEvent,
                    (const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>&),
                    (override));

        MOCK_METHOD(void,
                    registerProcessTerminationEvent,
                    (const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>&),
                    (override));

        MOCK_METHOD(std::shared_ptr<IBreakpoint>,
                    createBreakpoint,
                    (uint64_t, const ActiveProcessInformation&, const std::function<BpResponse(IInterruptEvent&)>&),
                    (override));

        MOCK_METHOD(std::unique_ptr<std::string>, getResultsDir, (), (const override));

        MOCK_METHOD(std::unique_ptr<ILogger>, newNamedLogger, (std::string_view name), (const, override));

        MOCK_METHOD(void, writeToFile, (const std::string&, const std::string&), (const override));

        MOCK_METHOD(void, writeToFile, (const std::string&, const std::vector<uint8_t>&), (const override));

        MOCK_METHOD(void, sendErrorEvent, (std::string_view), (const override));

        MOCK_METHOD(void, sendInMemDetectionEvent, (std::string_view), (const override));

        MOCK_METHOD(void,
                    initializePlugins,
                    ((const std::map<std::string, std::vector<std::string>, std::less<>>&)),
                    (override));

        MOCK_METHOD(void,
                    passProcessStartEventToRegisteredPlugins,
                    (std::shared_ptr<const ActiveProcessInformation>),
                    (override));

        MOCK_METHOD(void,
                    passProcessTerminationEventToRegisteredPlugins,
                    (std::shared_ptr<const ActiveProcessInformation>),
                    (override));

        MOCK_METHOD(void, unloadPlugins, (), (override));

        MOCK_METHOD(std::shared_ptr<IIntrospectionAPI>, getIntrospectionAPI, (), (const override));
    };
}
