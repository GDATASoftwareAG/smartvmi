#include "GRPCLogger.h"
#include "../RustHelper.h"
#include <iostream>
#include <vector>

namespace VmiCore
{
    GRPCLogger::GRPCLogger(::rust::Box<::logging::grpc::GrpcLogger> logger) : logger(std::move(logger)) {}

    void GRPCLogger::bind(const std::initializer_list<CxxLogField>& fields)
    {
        ::rust::Vec<::logging::LogField> rustFields;
        rustFields.reserve(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        logger->bind(std::move(rustFields));
    }

    void GRPCLogger::debug(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        try
        {
            logger->log_no_base_fields(::logging::Level::DEBUG, toRustStr(message), std::move(rustFields));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::info(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        try
        {
            logger->log_no_base_fields(::logging::Level::INFO, toRustStr(message), std::move(rustFields));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::warning(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        try
        {
            logger->log_no_base_fields(::logging::Level::WARN, toRustStr(message), std::move(rustFields));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::error(std::string_view message, const std::initializer_list<CxxLogField>& fields) const
    {
        auto rustFields = logger->clone_base_fields(fields.size());
        appendCxxFieldsToRustFields(rustFields, fields);
        try
        {
            logger->log_no_base_fields(::logging::Level::ERROR, toRustStr(message), std::move(rustFields));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }
}
