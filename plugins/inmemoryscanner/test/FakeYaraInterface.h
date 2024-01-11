#pragma once

#include <IYaraInterface.h>

namespace InMemoryScanner
{
    class FakeYaraInterface : public IYaraInterface
    {
      public:
        std::vector<Rule> scanMemory(VmiCore::addr_t regionBase,
                                     std::span<const VmiCore::MappedRegion> mappedRegions) override;

        bool max_threads_exceeded = false;

      private:
        int concurrentThreads = 0;
    };
}
