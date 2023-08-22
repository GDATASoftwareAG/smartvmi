#include "../src/lib/Tracer.h"
#include "../src/lib/os/windows/Library.h"
#include "mock_Config.h"
#include "mock_TracedProcess.h"
#include "mock_TracedProcessFactory.h"
#include <functional>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>

using testing::_;
using testing::NiceMock;
using testing::Return;
using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;

namespace ApiTracing
{
    namespace
    {
        constexpr pid_t tracedProcessPid = 420;
        constexpr pid_t untracedProcessPid = 4711;
        constexpr pid_t tracedChildPid = 42;
        constexpr std::string_view targetProcessName = "TraceMeNow.exe";
        constexpr std::string_view untracedProcessName = "DefinitelyNot.exe";
        constexpr std::string_view emptyConfigProcessName = "EmptyConfigProcess.exe";
        constexpr std::string_view dontTraceChildrenProcessName = "DontTraceMyChildEverAgain.exe";
    }

    class TracerTestFixture : public testing::Test
    {
      protected:
        std::unique_ptr<VmiCore::Plugin::MockPluginInterface> mockPluginInterface =
            std::make_unique<VmiCore::Plugin::MockPluginInterface>();
        std::shared_ptr<MockTracedProcessFactory> mockTracedProcessFactory =
            std::make_shared<MockTracedProcessFactory>();
        std::shared_ptr<Tracer> tracer;
        std::map<std::string, TracingProfile, std::less<>> tracingProfiles = {
            {std::string(targetProcessName), {std::string(targetProcessName), true, {}}},
            {std::string(emptyConfigProcessName), {std::string(emptyConfigProcessName), false, {}}},
            {std::string(dontTraceChildrenProcessName), {std::string(dontTraceChildrenProcessName), false, {}}}};

        void SetUp() override
        {
            ON_CALL(*mockPluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<VmiCore::MockLogger>>(); });

            std::unique_ptr<MockConfig> mockConfig = std::make_unique<MockConfig>();
            setupMockConfig(*mockConfig);

            tracer =
                std::make_shared<Tracer>(mockPluginInterface.get(), std::move(mockConfig), mockTracedProcessFactory);
        }

        void setupTracedProcessFactory(const std::shared_ptr<const ActiveProcessInformation>& tracedProcessInformation,
                                       std::function<void(MockTracedProcess&)> tracedProcessSetupFn = {})
        {
            if (tracedProcessSetupFn)
            {
                EXPECT_CALL(*mockTracedProcessFactory, createTracedProcess(tracedProcessInformation, _))
                    .WillOnce(
                        [tracedProcessSetupFn = std::move(tracedProcessSetupFn)](
                            const std::shared_ptr<const VmiCore::ActiveProcessInformation>&,
                            const TracingProfile& tracingProfile)
                        {
                            auto p = setupTracedProcess(tracingProfile);
                            tracedProcessSetupFn(*p);
                            return p;
                        });
            }
            else
            {
                ON_CALL(*mockTracedProcessFactory, createTracedProcess(tracedProcessInformation, _))
                    .WillByDefault([](const std::shared_ptr<const VmiCore::ActiveProcessInformation>&,
                                      const TracingProfile& tracingProfile)
                                   { return setupTracedProcess(tracingProfile); });
            }
        }

        static std::unique_ptr<MockTracedProcess> setupTracedProcess(const TracingProfile& profile)
        {
            auto mockTracedProcess = std::make_unique<MockTracedProcess>();

            ON_CALL(*mockTracedProcess, getTracingProfile()).WillByDefault(Return(profile));
            ON_CALL(*mockTracedProcess, traceChildren()).WillByDefault(Return(profile.traceChildren));

            return mockTracedProcess;
        }

        static std::shared_ptr<const VmiCore::ActiveProcessInformation>
        createActiveProcessInformation(std::string_view processName, pid_t processPid, pid_t parentPid)
        {
            return std::make_shared<VmiCore::ActiveProcessInformation>(
                0,
                0,
                0,
                processPid,
                parentPid,
                std::string(processName),
                std::make_unique<std::string>(processName),
                std::make_unique<std::string>(""),
                std::make_unique<VmiCore::MockMemoryRegionExtractor>(),
                true);
        }

        void setupMockConfig(const MockConfig& mockConfig)
        {
            for (const auto& profile : tracingProfiles)
            {
                ON_CALL(mockConfig, getTracingProfile(profile.second.name)).WillByDefault(Return(profile.second));
            }
        }
    };

    TEST_F(TracerTestFixture, traceProcess_untracedProcess_noTracedProcessRegistered)
    {
        auto untracedProcessInformation = createActiveProcessInformation(untracedProcessName, untracedProcessPid, 0);
        EXPECT_CALL(*mockTracedProcessFactory, createTracedProcess(_, _)).Times(0);

        ASSERT_NO_THROW(tracer->traceProcess(untracedProcessInformation));
    }

    TEST_F(TracerTestFixture, traceProcess_tracedProcess_tracedProcessRegistered)
    {
        auto tracedProcessInformation = createActiveProcessInformation(targetProcessName, tracedProcessPid, 0);
        EXPECT_CALL(*mockTracedProcessFactory, createTracedProcess(_, _)).Times(1);
        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessInformation));
    }

    TEST_F(TracerTestFixture, traceProcess_childWithoutTracingProfile_createTracedProcessWithParentTracingProfile)
    {
        auto tracedProcessInformation = createActiveProcessInformation(targetProcessName, tracedProcessPid, 0);
        auto untracedProcessWithTracedParentInformation =
            createActiveProcessInformation(untracedProcessName, tracedChildPid, tracedProcessPid);
        setupTracedProcessFactory(tracedProcessInformation);
        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessInformation));

        EXPECT_CALL(*mockTracedProcessFactory,
                    createTracedProcess(untracedProcessWithTracedParentInformation,
                                        tracingProfiles.at(*tracedProcessInformation->fullName)))
            .Times(1);

        ASSERT_NO_THROW(tracer->traceProcess(untracedProcessWithTracedParentInformation));
    }

    TEST_F(TracerTestFixture,
           traceProcess_childWithConflictingTracingProfile_createTracedProcessWithParentTracingProfile)
    {
        auto tracedProcessInformation = createActiveProcessInformation(targetProcessName, tracedProcessPid, 0);
        auto tracedProcessWithConflictingParentConfig =
            createActiveProcessInformation(emptyConfigProcessName, tracedChildPid, tracedProcessPid);
        setupTracedProcessFactory(tracedProcessInformation);
        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessInformation));

        EXPECT_CALL(*mockTracedProcessFactory,
                    createTracedProcess(tracedProcessWithConflictingParentConfig,
                                        tracingProfiles.at(*tracedProcessInformation->fullName)))
            .Times(1);

        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessWithConflictingParentConfig));
    }

    TEST_F(TracerTestFixture, traceProcess_processWithoutTraceChildren_onlyParentTraced)
    {
        auto dontTraceChildrenProcessInformation =
            createActiveProcessInformation(dontTraceChildrenProcessName, tracedProcessPid, 0);
        auto childOfDontTraceChildProcessInformation =
            createActiveProcessInformation(targetProcessName, tracedChildPid, tracedProcessPid);
        setupTracedProcessFactory(dontTraceChildrenProcessInformation);
        ASSERT_NO_THROW(tracer->traceProcess(dontTraceChildrenProcessInformation));

        EXPECT_CALL(*mockTracedProcessFactory, createTracedProcess(_, _)).Times(0);

        ASSERT_NO_THROW(tracer->traceProcess(childOfDontTraceChildProcessInformation));
    }

    TEST_F(TracerTestFixture, removeTracedProcess_unrelatedProcess_noChange)
    {
        auto tracedProcessInformation = createActiveProcessInformation(targetProcessName, tracedProcessPid, 0);
        setupTracedProcessFactory(tracedProcessInformation,
                                  [](MockTracedProcess& p) { EXPECT_CALL(p, removeHooks()).Times(0); });
        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessInformation));

        EXPECT_NO_THROW(
            tracer->removeTracedProcess(createActiveProcessInformation(untracedProcessName, untracedProcessPid, 0)));
    }

    TEST_F(TracerTestFixture, removeTracedProcess_tracedProcessTerminates_tracedProcessRemoved)
    {
        auto tracedProcessInformation = createActiveProcessInformation(targetProcessName, tracedProcessPid, 0);
        setupTracedProcessFactory(tracedProcessInformation,
                                  [](MockTracedProcess& p) { EXPECT_CALL(p, removeHooks()).Times(1); });
        ASSERT_NO_THROW(tracer->traceProcess(tracedProcessInformation));

        EXPECT_NO_THROW(tracer->removeTracedProcess(tracedProcessInformation));
        // This call indirectly verifies that the traced process has also been removed from any internal containers.
        // Otherwise removeHooks() would be called more than once.
        EXPECT_NO_THROW(tracer->removeTracedProcess(tracedProcessInformation));
    }

    TEST_F(TracerTestFixture, teardown_multipleTracedProcesses_allTracedProcessesRemoved)
    {
        for (const auto& processInfo : {createActiveProcessInformation(targetProcessName, tracedProcessPid, 0),
                                        createActiveProcessInformation("something", 123, tracedProcessPid),
                                        createActiveProcessInformation("something", 234, tracedProcessPid)})
        {
            setupTracedProcessFactory(processInfo,
                                      [](MockTracedProcess& p) { EXPECT_CALL(p, removeHooks()).Times(1); });
            ASSERT_NO_THROW(tracer->traceProcess(processInfo));
        }

        EXPECT_NO_THROW(tracer->teardown());
    }
}
