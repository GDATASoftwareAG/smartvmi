#pragma once

#include "Common.h"
#include "IYaraInterface.h"
#include <string>
#include <yara.h>

namespace InMemoryScanner
{
    class YaraInterface : public IYaraInterface
    {
      public:
        YaraInterface(const std::string& rulesFile, int scanTimeout);

        YaraInterface(const YaraInterface& other) = delete;

        YaraInterface(YaraInterface&& other) noexcept : rules(other.rules)
        {
            other.rules = nullptr;
        }

        YaraInterface& operator=(const YaraInterface& other) = delete;

        YaraInterface& operator=(YaraInterface&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            rules = other.rules;
            other.rules = nullptr;

            return *this;
        }

        ~YaraInterface() override;

        std::vector<Rule> scanMemory(std::span<const VmiCore::MappedRegion> mappedRegions) override;

      private:
        int scanTimeout;
        YR_RULES* rules = nullptr;

        static int yaraCallback(YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data);

        static int handleRuleMatch(YR_SCAN_CONTEXT* context, YR_RULE* rule, std::vector<Rule>* results);
    };
}
