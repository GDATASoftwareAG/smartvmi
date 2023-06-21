#include "../src/lib/config/TracingDefinitions.h"
#include "../src/lib/config/TracingTargetsParser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Contains;

namespace ApiTracing
{
    TEST(TracingTargetsParserTests, getTracingTargets_calcProcess_correctProfile)
    {
        auto tracingTargetsParser = std::make_unique<TracingTargetsParser>("testConfiguration.yaml");
        TracingProfile expectedTracingProfile{
            .name = "calc",
            .traceChilds = true,
            .modules = {{.name = "ntdll.dll", .functions = {{"function1"}, {"function2"}}},
                        {.name = "kernel32.dll", .functions = {{"kernelfunction1"}, {"kernelfunction2"}}}}};

        auto tracingProfile = tracingTargetsParser->getTracingProfile("calc.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST(TracingTargetsParserTests, getTracingTargets_processWithoutProfile_defaultProfile)
    {
        auto tracingTargetsParser = std::make_unique<TracingTargetsParser>("testConfiguration.yaml");
        TracingProfile expectedTracingProfile{
            .name = "default", .traceChilds = true, .modules = {{.name = "ntdll.dll", .functions = {{"function1"}}}}};

        auto tracingProfile = tracingTargetsParser->getTracingProfile("notepad.exe");

        EXPECT_EQ(tracingProfile, expectedTracingProfile);
    }

    TEST(TracingTargetsParserTests, getTracingTargets_unknownProcessName_nullopt)
    {
        auto tracingTargetsParser = std::make_unique<TracingTargetsParser>("testConfiguration.yaml");

        auto tracingProfile = tracingTargetsParser->getTracingProfile("unknown.exe");

        EXPECT_FALSE(tracingProfile);
    }
}
