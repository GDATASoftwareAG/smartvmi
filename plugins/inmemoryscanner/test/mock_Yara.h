#pragma once

#include "../src/YaraInterface.h"
#include <gmock/gmock.h>

class MockYara : public YaraInterface
{
  public:
    MOCK_METHOD(std::unique_ptr<std::vector<Rule>>, scanMemory, (std::vector<uint8_t> & buffer), (override));
};
