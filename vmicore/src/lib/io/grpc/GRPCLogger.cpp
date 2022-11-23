#include "GRPCLogger.h"
#include <cxxbridge/rust_grpc_server/src/bridge.rs.h>
#include <initializer_list>
#include <iostream>
#include <list>
#include <utility>

namespace VmiCore
{
    GRPCLogger::GRPCLogger(::rust::Box<::logging::grpc::GrpcLogger> logger) : logger(std::move(logger)) {}

    void GRPCLogger::bind(const std::initializer_list<rust::Box<::logging::LogField>>& fields)
    {
        logger->bind(::rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void GRPCLogger::debug(std::string_view message,
                           const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::DEBUG,
                        toRustStr(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::info(std::string_view message,
                          const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::INFO,
                        toRustStr(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::warning(std::string_view message,
                             const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::WARN,
                        toRustStr(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::error(std::string_view message,
                           const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::ERROR,
                        toRustStr(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }
}
