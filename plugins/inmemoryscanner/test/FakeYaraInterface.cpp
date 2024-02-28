#include "FakeYaraInterface.h"
#include <thread>
#include <yara/limits.h> // NOLINT(modernize-deprecated-headers)

namespace InMemoryScanner
{
    std::vector<Rule>
    FakeYaraInterface::scanMemory([[maybe_unused]] std::span<const VmiCore::MappedRegion> mappedRegions)
    {
        concurrentThreads++;
        if (concurrentThreads > YR_MAX_THREADS)
        {
            max_threads_exceeded = true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        concurrentThreads--;
        return {};
    }
}
