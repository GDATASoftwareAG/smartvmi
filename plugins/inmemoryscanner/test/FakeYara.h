#pragma once

#include "../src/YaraInterface.h"

class FakeYara : public YaraInterface
{
  public:
    std::unique_ptr<std::vector<Rule>> scanMemory(std::vector<uint8_t>& buffer) override;

    bool max_threads_exceeded = false;

  private:
    int concurrentThreads = 0;
};
