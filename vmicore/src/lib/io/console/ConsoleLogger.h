#ifndef VMICORE_CONSOLELOGGER_H
#define VMICORE_CONSOLELOGGER_H

#include "../ILogger.h"

namespace VmiCore
{
    class ConsoleLogger : public ILogger
    {
      public:
        explicit ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger);
        ~ConsoleLogger() override = default;

        void bind(const std::initializer_list<rust::Box<::logging::LogField>>& fields) override;

        inline void debug(const std::string_view& message) const override
        {
            debug(message, {});
        };
        void debug(const std::string_view& message,
                   const std::initializer_list<rust::Box<::logging::LogField>>& fields) const override;

        inline void info(const std::string_view& message) const override
        {
            info(message, {});
        };
        void info(const std::string_view& message,
                  const std::initializer_list<rust::Box<::logging::LogField>>& fields) const override;

        inline void warning(const std::string_view& message) const override
        {
            warning(message, {});
        };
        void warning(const std::string_view& message,
                     const std::initializer_list<rust::Box<::logging::LogField>>& fields) const override;

        inline void error(const std::string_view& message) const override
        {
            error(message, {});
        };
        void error(const std::string_view& message,
                   const std::initializer_list<rust::Box<::logging::LogField>>& fields) const override;

      private:
        ::rust::Box<::logging::console::ConsoleLogger> logger;
    };
}

#endif // VMICORE_CONSOLELOGGER_H
