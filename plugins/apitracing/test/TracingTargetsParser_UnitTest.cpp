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
        ProcessTracingConfig tracingConfig{
            .name = "calc.exe",
            .profile = {
                .name = "calc",
                .traceChilds = true,
                .modules = {{.name = "ntdll.dll", .functions = {{"function1"}, {"function2"}}},
                            {.name = "kernel32.dll", .functions = {{"kernelfunction1"}, {"kernelfunction2"}}}}}};

        auto tracingTargets = tracingTargetsParser->getTracingTargets();

        EXPECT_THAT(tracingTargets, Contains(tracingConfig));
    }

    TEST(TracingTargetsParserTests, getTracingTargets_processWithoutProfile_defaultProfile)
    {
        auto tracingTargetsParser = std::make_unique<TracingTargetsParser>("testConfiguration.yaml");
        ProcessTracingConfig tracingConfig{
            .name = "notepad.exe",
            .profile = {.name = "default",
                        .traceChilds = true,
                        .modules = {{.name = "ntdll.dll", .functions = {{"function1"}}}}}};

        auto tracingTargets = tracingTargetsParser->getTracingTargets();

        EXPECT_THAT(tracingTargets, Contains(tracingConfig));
    }
}
