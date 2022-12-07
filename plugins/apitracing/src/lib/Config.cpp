#include "Config.h"
#include <utility>

namespace ApiTracing
{
    Config::Config(std::filesystem::path tracingTargetsPath) : tracingTargetsPath(std::move(tracingTargetsPath)) {}

    std::filesystem::path Config::getTracingTargetsPath() const
    {
        return tracingTargetsPath;
    }
}
