#ifndef VMICORE_ILOGGING_H
#define VMICORE_ILOGGING_H

#include <cxx_rust_part/bridge.h>
#include <memory>
#include <string_view>
#include <vmicore/io/ILogger.h>

namespace VmiCore
{
    class ILogging
    {
      public:
        virtual ~ILogging() = default;

        virtual void start() = 0;

        virtual void stop(const uint64_t& timeoutMillis) = 0;

        [[nodiscard]] virtual std::unique_ptr<ILogger> newLogger() = 0;

        [[nodiscard]] virtual std::unique_ptr<ILogger> newNamedLogger(std::string_view name) = 0;

        virtual void setLogLevel(::logging::Level level) = 0;

      protected:
        ILogging() = default;
    };
}

#endif // VMICORE_ILOGGING_H
