#include "ConsoleLogger.h"

ConsoleLogger::ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger) : logger(std::move(logger)) {}

void ConsoleLogger::bind(std::initializer_list<::rust::Box<::logging::LogField>> fields)
{
    logger->bind(rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
}

void ConsoleLogger::debug(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const
{
    logger->log(::logging::Level::DEBUG,
                message,
                rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
}

void ConsoleLogger::info(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const
{

    logger->log(::logging::Level::INFO,
                message,
                rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
}

void ConsoleLogger::warning(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const
{
    logger->log(::logging::Level::WARN,
                message,
                rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
}

void ConsoleLogger::error(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const
{
    logger->log(::logging::Level::ERROR,
                message,
                rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
}
