#pragma once

#include "../src/Dumping.h"
#include <gmock/gmock.h>

namespace InMemoryScanner
{
    class MockDumping : public IDumping
    {
      public:
        MOCK_METHOD(void,
                    dumpMemoryRegion,
                    (const std::string& processName,
                     pid_t pid,
                     const VmiCore::MemoryRegion& memoryRegionDescriptor,
                     const std::vector<uint8_t>& data),
                    (override));

        MOCK_METHOD(std::vector<std::string>, getAllMemoryRegionInformation, (), (override));
    };
}
