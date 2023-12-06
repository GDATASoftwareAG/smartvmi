#ifndef VMICORE_ILOGGER_H
#define VMICORE_ILOGGER_H

#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <variant>

namespace VmiCore
{
    constexpr auto WRITE_TO_FILE_TAG = "writeToFileTag";

    using CxxLogField = std::pair<std::string_view, std::variant<std::string_view, bool, int64_t, uint64_t, double>>;

    class ILogger
    {
      public:
        virtual ~ILogger() = default;
        virtual void bind(const std::initializer_list<CxxLogField>& fields) = 0;

        virtual void debug(std::string_view message) const = 0;
        virtual void debug(std::string_view message, const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void info(std::string_view message) const = 0;
        virtual void info(std::string_view message, const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void warning(std::string_view message) const = 0;
        virtual void warning(std::string_view message, const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void error(std::string_view message) const = 0;
        virtual void error(std::string_view message, const std::initializer_list<CxxLogField>& fields) const = 0;

      protected:
        ILogger() = default;
    };
}

#endif // VMICORE_ILOGGER_H
