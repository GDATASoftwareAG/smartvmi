#pragma once

#include "Common.h"
#include <memory>
#include <vmicore/vmi/IMemoryMapping.h>

namespace InMemoryScanner
{
    class YaraException : public std::runtime_error
    {
      public:
        explicit YaraException(const std::string& Message) : std::runtime_error(Message.c_str()){};
    };

    class YaraInterface
    {
      public:
        virtual ~YaraInterface() = default;

        virtual std::unique_ptr<std::vector<Rule>>
        scanMemory(const std::vector<VmiCore::MappedRegion>& mappedRegions) = 0;

      protected:
        YaraInterface() = default;
    };
}
