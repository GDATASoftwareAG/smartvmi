#pragma once

#include <IYaraInterface.h>
#include <gmock/gmock.h>

namespace InMemoryScanner
{
    class MockYaraInterface : public IYaraInterface
    {
      public:
        MOCK_METHOD(std::unique_ptr<std::vector<Rule>>,
                    scanMemory,
                    (const std::vector<VmiCore::MappedRegion>& mappedRegions),
                    (override));
    };
}
