#ifndef INMEMORYSCANNER_IYARAINTERFACE_H
#define INMEMORYSCANNER_IYARAINTERFACE_H

#include "Common.h"
#include <span>
#include <stdexcept>
#include <vector>
#include <vmicore/vmi/IMemoryMapping.h>

namespace InMemoryScanner
{
    class YaraException : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class YaraTimeoutException final : public YaraException
    {
        using YaraException::YaraException;
    };

    class IYaraInterface
    {
      public:
        virtual ~IYaraInterface() = default;

        virtual std::vector<Rule> scanMemory(std::span<const VmiCore::MappedRegion> mappedRegions) = 0;

      protected:
        IYaraInterface() = default;
    };
}

#endif // INMEMORYSCANNER_IYARAINTERFACE_H
