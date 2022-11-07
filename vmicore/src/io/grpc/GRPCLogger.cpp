#include "GRPCLogger.h"
#include "GRPCServer.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <initializer_list>
#include <iostream>
#include <list>
#include <memory>
#include <utility>

namespace VmiCore
{
    GRPCLogger::GRPCLogger(::rust::Box<::logging::grpc::GrpcLogger> logger) : logger(std::move(logger)) {}

    void GRPCLogger::bind(const std::initializer_list<rust::Box<::logging::LogField>>& fields)
    {
        logger->bind(::rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
    }

    void GRPCLogger::debug(const std::string_view& message,
                           const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::DEBUG,
                        static_cast<std::string>(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::info(const std::string_view& message,
                          const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::INFO,
                        static_cast<std::string>(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::warning(const std::string_view& message,
                             const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::WARN,
                        static_cast<std::string>(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }

    void GRPCLogger::error(const std::string_view& message,
                           const std::initializer_list<rust::Box<::logging::LogField>>& fields) const
    {
        try
        {
            logger->log(::logging::Level::ERROR,
                        static_cast<std::string>(message),
                        rust::Slice<const ::rust::Box<::logging::LogField>>(std::data(fields), fields.size()));
        }
        catch (const ::rust::Error& e)
        {
            std::cerr << "could not write log: " << e.what() << std::endl;
        }
    }
}
