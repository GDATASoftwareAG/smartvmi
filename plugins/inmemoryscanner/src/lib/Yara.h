#pragma once

#include "YaraInterface.h"
#include <yara.h>

namespace InMemoryScanner
{
    class Yara : public YaraInterface
    {
      public:
        explicit Yara(const std::string& rulesFile);

        ~Yara() override;

        std::unique_ptr<std::vector<Rule>> scanMemory(std::vector<uint8_t>& buffer) override;

      private:
        YR_RULES* rules = nullptr;

        [[nodiscard]] static int
        yaraCallback(YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data);

        [[nodiscard]] static int handleRuleMatch(YR_SCAN_CONTEXT* context, YR_RULE* rule, std::vector<Rule>* results);
    };
}
