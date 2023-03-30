#ifndef VMICORE_GRPCLOGGER_H
#define VMICORE_GRPCLOGGER_H

#include <cxxbridge/rust/cxx.h>
#include <cxxbridge/rust_grpc_server/src/bridge.rs.h>
#include <vmicore/io/ILogger.h>

namespace VmiCore
{
    class GRPCLogger : public ILogger
    {
      public:
        explicit GRPCLogger(rust::Box<::logging::grpc::GrpcLogger> logger);
        ~GRPCLogger() override = default;

        void bind(const std::initializer_list<CxxLogField>& fields) override;

        inline void debug(std::string_view message) const override
        {
            debug(message, {});
        };
        void debug(std::string_view message, const std::initializer_list<CxxLogField>& fields) const override;

        inline void info(std::string_view message) const override
        {
            info(message, {});
        };
        void info(std::string_view message, const std::initializer_list<CxxLogField>& fields) const override;

        inline void warning(std::string_view message) const override
        {
            warning(message, {});
        };
        void warning(std::string_view message, const std::initializer_list<CxxLogField>& fields) const override;

        inline void error(std::string_view message) const override
        {
            error(message, {});
        };
        void error(std::string_view message, const std::initializer_list<CxxLogField>& fields) const override;

      private:
        ::rust::Box<::logging::grpc::GrpcLogger> logger;
    };
}

#endif // VMICORE_GRPCLOGGER_H
