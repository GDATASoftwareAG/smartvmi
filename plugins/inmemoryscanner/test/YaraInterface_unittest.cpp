#include <YaraInterface.h>
#include <fmt/core.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string_view>

using testing::UnorderedElementsAre;
using VmiCore::PagingDefinitions::pageSizeInBytes;

namespace InMemoryScanner
{
    std::vector<uint8_t> constructPageWithContent(const std::string& string, bool insertAtBack = false)
    {
        std::vector<uint8_t> result(pageSizeInBytes, 0);
        auto insertPosition = insertAtBack ? result.end() - string.size() : result.begin();
        std::copy(string.begin(), string.end(), insertPosition);
        return result;
    }

    std::string compileYaraRules(std::string_view rules)
    {
        auto fileName = fmt::format("{}.sigs", testing::UnitTest::GetInstance()->current_test_info()->name());

        if (yr_initialize() != ERROR_SUCCESS)
        {
            throw std::runtime_error("Unable to initialize libyara");
        }
        YR_COMPILER* compiler = nullptr;
        if (yr_compiler_create(&compiler) != ERROR_SUCCESS)
        {
            throw std::runtime_error("Unable to create yara compiler");
        }
        if (auto syntax_errors = yr_compiler_add_string(compiler, rules.data(), nullptr) > 0)
        {
            throw std::runtime_error(
                fmt::format("Compiled rules are faulty. Number of syntax errors: {}", syntax_errors));
        }
        YR_RULES* compiled_rules = nullptr;
        if (yr_compiler_get_rules(compiler, &compiled_rules))
        {
            throw std::runtime_error("Unable to obtain rules from compiler");
        }
        if (yr_rules_save(compiled_rules, fileName.c_str()) != ERROR_SUCCESS)
        {
            throw std::runtime_error("Unable to save rules to file");
        }
        yr_compiler_destroy(compiler);
        yr_finalize();

        return fileName;
    }

    TEST(YaraTest, scanMemory_MissingMemoryRegion_NoMatch)
    {
        auto* rules = R"(
                        rule testRule
                        {
                            strings:
                                $test = "ABCD"
                                $test2 = "DCBA"

                            condition:
                                all of them
                        }
                    )";
        auto yaraInterface = YaraInterface(compileYaraRules(rules));
        auto subRegion1 = constructPageWithContent("ABCD");
        std::vector<VmiCore::MappedRegion> memoryRegions{{0x0, subRegion1}};

        auto matches = yaraInterface.scanMemory(memoryRegions);

        EXPECT_EQ(matches.size(), 0);
    }

    TEST(YaraTest, scanMemory_StringSplitInHalfThroughSubRegionBoundary_NoMatch)
    {
        auto* rules = R"(
                        rule testRule
                        {
                            strings:
                                $test = "CDDC"

                            condition:
                                all of them
                        }
                    )";
        auto yaraInterface = YaraInterface(compileYaraRules(rules));
        auto subRegion1 = constructPageWithContent("ABCD", true);
        auto subRegion2 = constructPageWithContent("DCBA", false);
        std::vector<VmiCore::MappedRegion> memoryRegions{{0x0, subRegion1}, {pageSizeInBytes, subRegion2}};

        auto matches = yaraInterface.scanMemory(memoryRegions);

        EXPECT_EQ(matches.size(), 0);
    }

    TEST(YaraTest, scanMemory_AllOfConditionStringsInDifferentRegions_NoMatch)
    {
        auto* rules = R"(
                        rule testRule
                        {
                            strings:
                                $test = "ABCD"
                                $test2 = "DCBA"

                            condition:
                                all of them
                        }
                    )";
        auto yaraInterface = YaraInterface(compileYaraRules(rules));
        auto subRegion1 = constructPageWithContent("ABCD");
        auto subRegion2 = constructPageWithContent("DCBA");
        std::vector<VmiCore::MappedRegion> memoryRegion1{{0x0, subRegion1}};
        std::vector<VmiCore::MappedRegion> memoryRegion2{{4 * pageSizeInBytes, subRegion2}};

        auto matches1 = yaraInterface.scanMemory(memoryRegion1);
        auto matches2 = yaraInterface.scanMemory(memoryRegion2);

        EXPECT_EQ(matches1.size(), 0);
        EXPECT_EQ(matches2.size(), 0);
    }

    TEST(YaraTest, scanMemory_AllOfConditionStringsInDifferentSubRegions_Matches)
    {
        auto* rules = R"(
                        rule testRule
                        {
                            strings:
                                $test = "ABCD"
                                $test2 = "DCBA"

                            condition:
                                all of them
                        }
                    )";
        auto yaraInterface = YaraInterface(compileYaraRules(rules));
        auto subRegion1 = constructPageWithContent("ABCD");
        auto subRegion2 = constructPageWithContent("DCBA");
        std::vector<VmiCore::MappedRegion> memoryRegions{{0x0, subRegion1}, {4 * pageSizeInBytes, subRegion2}};
        Rule expectedMatch{"testRule", "default", {{"$test", 0x0}, {"$test2", 4 * pageSizeInBytes}}};

        auto matches = yaraInterface.scanMemory(memoryRegions);

        ASSERT_EQ(matches.size(), 1);
        EXPECT_THAT(matches, UnorderedElementsAre(expectedMatch));
    }

    TEST(YaraTest, scanMemory_DifferentRulesInRegions_BothMatch)
    {
        auto* rules = R"(
                        rule testRule
                        {
                            strings:
                                $test = "ABCD"
                                $test2 = "DCBA"

                            condition:
                                all of them
                        }

                        rule testRule2
                        {
                            strings:
                                $test = "E"
                                $test2 = "F"

                            condition:
                                all of them
                        }
                    )";
        auto yaraInterface = YaraInterface(compileYaraRules(rules));
        auto subRegion1 = constructPageWithContent("ABCD");
        auto subRegion2 = constructPageWithContent("DCBA");
        auto subRegion3 = constructPageWithContent("EFGH");
        std::vector<VmiCore::MappedRegion> memoryRegions{
            {0x0, subRegion1}, {4 * pageSizeInBytes, subRegion2}, {8 * pageSizeInBytes, subRegion3}};
        Rule expectedMatch1{"testRule", "default", {{"$test", 0x0}, {"$test2", 4 * pageSizeInBytes}}};
        Rule expectedMatch2{
            "testRule2", "default", {{"$test", 8 * pageSizeInBytes}, {"$test2", 8 * pageSizeInBytes + 1}}};

        auto matches = yaraInterface.scanMemory(memoryRegions);

        ASSERT_EQ(matches.size(), 2);
        EXPECT_THAT(matches, UnorderedElementsAre(expectedMatch1, expectedMatch2));
    }
}
