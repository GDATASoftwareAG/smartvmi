#include "ConsoleLogger.h"

namespace VmiCore
{
    ConsoleLogger::ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger) : logger(std::move(logger)) {}

    void ConsoleLogger::bind(const std::initializer_list<rust::Box<::logging::LogField>>& fields)
    {
        logger->bind(rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::debug(const std::string_view& message,
                              const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::DEBUG,
                    static_cast<std::string>(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::info(const std::string_view& message,
                             const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {

        logger->log(::logging::Level::INFO,
                    static_cast<std::string>(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::warning(const std::string_view& message,
                                const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::WARN,
                    static_cast<std::string>(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void ConsoleLogger::error(const std::string_view& message,
                              const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        logger->log(::logging::Level::ERROR,
                    static_cast<std::string>(message),
                    rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }
}
