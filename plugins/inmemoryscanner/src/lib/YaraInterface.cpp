#include "YaraInterface.h"
#include <fmt/core.h>

using VmiCore::addr_t;
using VmiCore::MappedRegion;
using VmiCore::PagingDefinitions::pageSizeInBytes;

namespace
{
    struct YaraIteratorContext
    {
        std::vector<YR_MEMORY_BLOCK> blocks;
        std::size_t index;
    };

    YR_MEMORY_BLOCK* get_next_block(YR_MEMORY_BLOCK_ITERATOR* iterator)
    {
        if (auto* iteratorContext = static_cast<YaraIteratorContext*>(iterator->context);
            ++iteratorContext->index < iteratorContext->blocks.size())
        {
            return &iteratorContext->blocks[iteratorContext->index];
        }

        return nullptr;
    }

    YR_MEMORY_BLOCK* get_first_block(YR_MEMORY_BLOCK_ITERATOR* iterator)
    {
        auto* iteratorContext = static_cast<YaraIteratorContext*>(iterator->context);
        iteratorContext->index = 0;

        return &iteratorContext->blocks[iteratorContext->index];
    }

    const uint8_t* fetch_block_data(YR_MEMORY_BLOCK* block)
    {
        return static_cast<const uint8_t*>(block->context);
    }
}

namespace InMemoryScanner
{
    YaraInterface::YaraInterface(const std::string& rulesFile)
    {
        auto err = yr_initialize();
        if (err != ERROR_SUCCESS)
        {
            throw YaraException(fmt::format("Cannot initialize Yara. Error code: {}", err));
        }

        err = yr_rules_load(rulesFile.c_str(), &rules);
        if (err != ERROR_SUCCESS)
        {
            throw YaraException(fmt::format("Cannot load rules. Error code: {}", err));
        }
    }

    YaraInterface::~YaraInterface()
    {
        if (rules)
        {
            yr_rules_destroy(rules);
            yr_finalize();
        }
    }

    std::vector<Rule> YaraInterface::scanMemory(std::span<const MappedRegion> mappedRegions)
    {
        std::vector<Rule> results;

        YaraIteratorContext iteratorContext{};
        iteratorContext.blocks.reserve(mappedRegions.size());
        for (const auto& mappedRegion : mappedRegions)
        {
            iteratorContext.blocks.emplace_back(mappedRegion.num_pages * pageSizeInBytes,
                                                mappedRegion.guestBaseVA,
                                                mappedRegion.mappingBase,
                                                &fetch_block_data);
        }

        YR_MEMORY_BLOCK_ITERATOR iterator{.context = &iteratorContext,
                                          .first = &get_first_block,
                                          .next = &get_next_block,
                                          .file_size = nullptr,
                                          .last_error = ERROR_SUCCESS};

        if (auto err = yr_rules_scan_mem_blocks(rules,
                                                &iterator,
                                                SCAN_FLAGS_PROCESS_MEMORY | SCAN_FLAGS_REPORT_RULES_MATCHING,
                                                yaraCallback,
                                                &results,
                                                0);
            err != ERROR_SUCCESS)
        {
            throw YaraException(fmt::format("Error scanning memory. Error code: {}", err));
        }

        return results;
    }

    int YaraInterface::yaraCallback(YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data)
    {
        int ret = 0;
        switch (message)
        {
            case CALLBACK_MSG_RULE_MATCHING:
                ret = handleRuleMatch(
                    context, static_cast<YR_RULE*>(message_data), static_cast<std::vector<Rule>*>(user_data));
                break;
            case CALLBACK_MSG_RULE_NOT_MATCHING:
                [[fallthrough]];
            case CALLBACK_MSG_SCAN_FINISHED:
                ret = CALLBACK_CONTINUE;
                break;
            default:
                ret = CALLBACK_ERROR;
                break;
        }

        return ret;
    }

    int YaraInterface::handleRuleMatch(YR_SCAN_CONTEXT* context, YR_RULE* rule, std::vector<Rule>* results)
    {
        YR_STRING* string = nullptr;
        YR_MATCH* match = nullptr;

        Rule tmpRule;
        tmpRule.ruleName = rule->identifier;
        tmpRule.ruleNamespace = rule->ns->name;

        yr_rule_strings_foreach(rule, string)
        {
            yr_string_matches_foreach(context, string, match) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            {
                Match tmpMatch;
                tmpMatch.matchName = string->identifier;
                tmpMatch.position = match->base + match->offset;

                tmpRule.matches.push_back(tmpMatch);
            }
        }

        results->push_back(tmpRule);

        return CALLBACK_CONTINUE;
    }
}
