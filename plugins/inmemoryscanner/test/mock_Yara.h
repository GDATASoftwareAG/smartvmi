#pragma once

#include <YaraInterface.h>
#include <gmock/gmock.h>

namespace InMemoryScanner
{
    class MockYara : public YaraInterface
    {
      public:
        MOCK_METHOD(std::unique_ptr<std::vector<Rule>>,
                    scanMemory,
                    (const std::vector<VmiCore::MappedRegion>& mappedRegions),
                    (override));
    };
}
