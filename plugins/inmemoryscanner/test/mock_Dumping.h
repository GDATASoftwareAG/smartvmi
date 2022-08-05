#pragma once

#include "../src/Dumping.h"
#include <gmock/gmock.h>

class MockDumping : public IDumping
{
  public:
    MOCK_METHOD(void,
                dumpMemoryRegion,
                (const std::string& processName,
                 pid_t pid,
                 const MemoryRegion& memoryRegionDescriptor,
                 const std::vector<uint8_t>& data),
                (override));

    MOCK_METHOD(std::vector<std::string>, getAllMemoryRegionInformation, (), (override));
};
