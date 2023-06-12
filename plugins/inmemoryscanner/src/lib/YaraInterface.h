#pragma once

#include "Common.h"
#include "IYaraInterface.h"
#include <memory>
#include <string>
#include <vector>
#include <yara.h>

namespace InMemoryScanner
{
    class YaraInterface : public IYaraInterface
    {
      public:
        explicit YaraInterface(const std::string& rulesFile);

        ~YaraInterface() override;

        std::unique_ptr<std::vector<Rule>> scanMemory(const std::vector<VmiCore::MappedRegion>& mappedRegions) override;

      private:
        YR_RULES* rules = nullptr;

        static int yaraCallback(YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data);

        static int handleRuleMatch(YR_SCAN_CONTEXT* context, YR_RULE* rule, std::vector<Rule>* results);
    };
}
