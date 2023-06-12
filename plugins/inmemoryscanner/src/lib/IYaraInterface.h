#ifndef INMEMORYSCANNER_IYARAINTERFACE_H
#define INMEMORYSCANNER_IYARAINTERFACE_H

#include "Common.h"
#include <exception>
#include <string>
#include <vector>
#include <vmicore/vmi/IMemoryMapping.h>

namespace InMemoryScanner
{
    class YaraException : public std::runtime_error
    {
      public:
        explicit YaraException(const std::string& Message) : std::runtime_error(Message.c_str()){};
    };

    class IYaraInterface
    {
      public:
        virtual ~IYaraInterface() = default;

        virtual std::unique_ptr<std::vector<Rule>>
        scanMemory(const std::vector<VmiCore::MappedRegion>& mappedRegions) = 0;

      protected:
        IYaraInterface() = default;
    };
}

#endif // INMEMORYSCANNER_IYARAINTERFACE_H
