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
        ::rust::Box<::logging::grpc::GrpcLogger> logger;
    };
}

#endif // VMICORE_GRPCLOGGER_H
