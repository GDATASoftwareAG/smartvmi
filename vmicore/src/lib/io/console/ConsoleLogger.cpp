#include "ConsoleLogger.h"

namespace VmiCore
{
    ConsoleLogger::ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger) : logger(std::move(logger)) {}

    void ConsoleLogger::bind(const std::initializer_list<rust::Box<::logging::LogField>>& fields)
    {
        logger->bind(rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::debug(std::string_view message,
                              const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::DEBUG,
                    toRustStr(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::info(std::string_view message,
                             const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {

        logger->log(::logging::Level::INFO,
                    toRustStr(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::warning(std::string_view message,
                                const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::WARN,
                    toRustStr(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::error(std::string_view message,
                              const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::ERROR,
                    toRustStr(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }
}
