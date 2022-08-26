#ifndef VMICORE_ILOGGING_H
#define VMICORE_ILOGGING_H

#include "ILogger.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <memory>
#include <string>

namespace VmiCore
{
    class ILogging
    {
      public:
        virtual ~ILogging() = default;
        virtual void start() = 0;
        virtual void stop(const uint64_t& timeoutMillis) = 0;
        virtual std::unique_ptr<ILogger> newLogger() = 0;
        virtual std::unique_ptr<ILogger> newNamedLogger(const std::string& name) = 0;
        virtual void setLogLevel(::logging::Level level) = 0;

      protected:
        ILogging() = default;
    };
}

#endif // VMICORE_ILOGGING_H
