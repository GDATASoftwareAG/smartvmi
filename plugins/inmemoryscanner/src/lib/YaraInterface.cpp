#include "YaraInterface.h"
#include <fmt/core.h>

using VmiCore::MappedRegion;
using BlockIteratorPair = std::pair<std::vector<YR_MEMORY_BLOCK>::iterator, std::vector<YR_MEMORY_BLOCK>::iterator>;

namespace InMemoryScanner
{
    YaraInterface::YaraInterface(const std::string& rulesFile)
    {
        auto err = yr_initialize();
        if (err != ERROR_SUCCESS)
        {
            throw YaraException("Cannot initialize Yara. Error code: " + std::to_string(err));
        }

        err = yr_rules_load(rulesFile.c_str(), &rules);
        if (err != ERROR_SUCCESS)
        {
            throw YaraException("Cannot load rules. Error code: " + std::to_string(err));
        }
    }

    YaraInterface::~YaraInterface()
    {
        yr_rules_destroy(rules);
        yr_finalize();
    }

    YR_MEMORY_BLOCK* get_next_block(YR_MEMORY_BLOCK_ITERATOR* iterator)
    {
        auto* blockVectorIterators = reinterpret_cast<BlockIteratorPair*>(iterator->context);
        blockVectorIterators->first++;

        if (blockVectorIterators->first == blockVectorIterators->second)
        {
            return nullptr;
        }

        return &*blockVectorIterators->first;
    }

    YR_MEMORY_BLOCK* get_first_block(YR_MEMORY_BLOCK_ITERATOR* iterator)
    {
        return &*reinterpret_cast<BlockIteratorPair*>(iterator->context)->first;
    }

    const uint8_t* fetch_block_data(YR_MEMORY_BLOCK* block)
    {
        return reinterpret_cast<const uint8_t*>(block->context);
    }

    std::unique_ptr<std::vector<Rule>> YaraInterface::scanMemory(const std::vector<MappedRegion>& mappedRegions)
    {
        auto results = std::make_unique<std::vector<Rule>>();

        std::vector<YR_MEMORY_BLOCK> blocks;
        blocks.reserve(mappedRegions.size());
        for (const auto& mappedRegion : mappedRegions)
        {
            blocks.emplace_back(mappedRegion.mapping.size(),
                                mappedRegion.guestBaseVA - mappedRegions.front().guestBaseVA,
                                reinterpret_cast<void*>(mappedRegion.mapping.data()),
                                &fetch_block_data);
        }
        auto blockIterators = std::make_pair(blocks.begin(), blocks.end());
#ifdef LIBYARA_4_1
        YR_MEMORY_BLOCK_ITERATOR iterator{.context = &blockIterators,
                                          .first = &get_first_block,
                                          .next = &get_next_block,
                                          .file_size = nullptr,
                                          .last_error = ERROR_SUCCESS};
#else
        YR_MEMORY_BLOCK_ITERATOR iterator{
            .context = &blockIterators, .first = &get_first_block, .next = &get_next_block};
#endif

        auto err = yr_rules_scan_mem_blocks(rules, &iterator, 0, yaraCallback, results.get(), 0);
        if (err != ERROR_SUCCESS)
        {
            throw YaraException("Error scanning memory. Error code: " + std::to_string(err));
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
