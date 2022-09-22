#include "Yara.h"

namespace InMemoryScanner
{
    Yara::Yara(const std::string& rulesFile)
    {
        int err = 0;

        err = yr_initialize();
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

    Yara::~Yara()
    {
        yr_rules_destroy(rules);
        yr_finalize();
    }

    std::unique_ptr<std::vector<Rule>> Yara::scanMemory(std::vector<uint8_t>& buffer)
    {
        auto results = std::make_unique<std::vector<Rule>>();
        int err = 0;

        err = yr_rules_scan_mem(rules, buffer.data(), buffer.size(), 0, yaraCallback, results.get(), 0);
        if (err != ERROR_SUCCESS)
        {
            throw YaraException("Error scanning memory. Error code: " + std::to_string(err));
        }

        return results;
    }

    int Yara::yaraCallback(YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data)
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

    int Yara::handleRuleMatch(YR_SCAN_CONTEXT* context, YR_RULE* rule, std::vector<Rule>* results)
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
                tmpMatch.position = match->offset;

                tmpRule.matches.push_back(tmpMatch);
            }
        }

        results->push_back(tmpRule);

        return CALLBACK_CONTINUE;
    }
}
