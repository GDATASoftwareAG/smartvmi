#ifndef VMICORE_ILOGGER_H
#define VMICORE_ILOGGER_H

#include <memory>
#include <string_view>
#include <variant>

namespace VmiCore
{
    // Has to be the same as in the HiveOperations project to indicate in which file the log should be written.
    // pkg->script->vmi->vmi_connector.go
    constexpr auto WRITE_TO_FILE_TAG = "writeToFileTag";

    using CxxLogField = std::pair<std::string_view, std::variant<std::string_view, bool, int64_t, uint64_t, double>>;

    class ILogger
    {
      public:
        virtual ~ILogger() = default;
        virtual void bind(const std::initializer_list<CxxLogField>& fields) = 0;

        virtual void debug(std::string_view message) const = 0;
        virtual void debug(std::string_view message,
                           const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void info(std::string_view message) const = 0;
        virtual void info(std::string_view message,
                          const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void warning(std::string_view message) const = 0;
        virtual void warning(std::string_view message,
                             const std::initializer_list<CxxLogField>& fields) const = 0;

        virtual void error(std::string_view message) const = 0;
        virtual void error(std::string_view message,
                           const std::initializer_list<CxxLogField>& fields) const = 0;

      protected:
        ILogger() = default;
    };
}

#endif // VMICORE_ILOGGER_H
