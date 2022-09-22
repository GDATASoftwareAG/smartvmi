#include "FakeYara.h"
#include <thread>
#include <yara/limits.h> // NOLINT(modernize-deprecated-headers)

namespace InMemoryScanner
{
    std::unique_ptr<std::vector<Rule>> FakeYara::scanMemory([[maybe_unused]] std::vector<uint8_t>& buffer)
    {
        concurrentThreads++;
        if (concurrentThreads > YR_MAX_THREADS)
        {
            max_threads_exceeded = true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        concurrentThreads--;
        return std::make_unique<std::vector<Rule>>();
    }
}
