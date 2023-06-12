#pragma once

#include <IYaraInterface.h>

namespace InMemoryScanner
{
    class FakeYaraInterface : public IYaraInterface
    {
      public:
        std::unique_ptr<std::vector<Rule>> scanMemory(const std::vector<VmiCore::MappedRegion>& mappedRegions) override;

        bool max_threads_exceeded = false;

      private:
        int concurrentThreads = 0;
    };
}
