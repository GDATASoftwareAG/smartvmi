#ifndef VMICORE_GRPCLOGGER_H
#define VMICORE_GRPCLOGGER_H

#include "../ILogger.h"
#include "cxxbridge/rust/cxx.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <initializer_list>
#include <memory>
#include <string>

namespace VmiCore
{
    class GRPCLogger : public ILogger
    {
      public:
        explicit GRPCLogger(rust::Box<::logging::grpc::GrpcLogger> logger);
        ~GRPCLogger() override = default;

        void bind(std::initializer_list<::rust::Box<::logging::LogField>> fields) override;

        inline void debug(std::string message) const override
        {
            debug(message, {});
        };
        void debug(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

        inline void info(std::string message) const override
        {
            info(message, {});
        };
        void info(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

        inline void warning(std::string message) const override
        {
            warning(message, {});
        };
        void warning(std::string message,
                     std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

        inline void error(std::string message) const override
        {
            error(message, {});
        };
        void error(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

      private:
        ::rust::Box<::logging::grpc::GrpcLogger> logger;
    };
}

#endif // VMICORE_GRPCLOGGER_H
