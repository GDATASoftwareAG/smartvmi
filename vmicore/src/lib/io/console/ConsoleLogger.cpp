#include "ConsoleLogger.h"
#include "../RustHelper.h"

namespace VmiCore
{
    ConsoleLogger::ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger) : logger(std::move(logger)) {}

    void ConsoleLogger::bind(const std::initializer_list<CxxLogField>& fields)
    {
        ::rust::Vec<::logging::LogField> rustFields;
        rustFields.reserve(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->bind(std::move(rustFields));
    }

    void ConsoleLogger::debug(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->log_no_base_fields(::logging::Level::DEBUG, toRustStr(message), std::move(rustFields));
    }

    void ConsoleLogger::info(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->log_no_base_fields(::logging::Level::INFO, toRustStr(message), std::move(rustFields));
    }

    void ConsoleLogger::warning(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->log_no_base_fields(::logging::Level::WARN, toRustStr(message), std::move(rustFields));
    }

    void ConsoleLogger::error(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->log_no_base_fields(::logging::Level::ERROR, toRustStr(message), std::move(rustFields));
    }
}
