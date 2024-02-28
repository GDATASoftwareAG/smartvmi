#pragma once

#include <IYaraInterface.h>
#include <gmock/gmock.h>

namespace InMemoryScanner
{
    class MockYaraInterface : public IYaraInterface
    {
      public:
        MOCK_METHOD(std::vector<Rule>, scanMemory, (std::span<const VmiCore::MappedRegion>), (override));
    };
}
