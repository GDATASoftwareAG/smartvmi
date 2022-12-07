#include "../src/lib/config/TracingDefinitions.h"
#include "../src/lib/config/TracingTargetsParser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::UnorderedElementsAre;

namespace ApiTracing
{
    TEST(TracingTargetsParserTests, getTracingTargets_validYaml_correctTargets)
    {
        auto tracingTargetsParser = std::make_unique<TracingTargetsParser>();
        ProcessInformation tracingInformation{
            .traceChilds = true,
            .name = "calc.exe",
            .modules = {
                ModuleInformation{.name = "ntdll.dll", .functions = {{"function1"}, {"function2"}}},
                ModuleInformation{.name = "kernel32.dll", .functions = {{"kernelfunction1"}, {"kernelfunction2"}}}}};

        auto tracingTargets = tracingTargetsParser->getTracingTargets("testConfiguration.yaml");

        EXPECT_THAT(*tracingTargets, UnorderedElementsAre(tracingInformation));
    }
}
