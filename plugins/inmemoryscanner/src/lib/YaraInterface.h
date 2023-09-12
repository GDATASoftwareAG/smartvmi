#pragma once

#include "Common.h"
#include <memory>

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

        [[nodiscard]] virtual std::unique_ptr<std::vector<Rule>> scanMemory(std::vector<uint8_t>& buffer) = 0;

      protected:
        YaraInterface() = default;
    };
}
