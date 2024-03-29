#ifndef VMICORE_CONSOLELOGGERBUILDER_H
#define VMICORE_CONSOLELOGGERBUILDER_H

#include "../ILogging.h"

namespace VmiCore
{
    class ConsoleLoggerBuilder : public ILogging
    {
      public:
        explicit ConsoleLoggerBuilder(
            std::shared_ptr<::rust::Box<::logging::console::ConsoleLoggerBuilder>> consoleLoggerBuilder);
        ~ConsoleLoggerBuilder() override = default;

        void start() override {}

        void stop(const uint64_t&) override {}

        [[nodiscard]] std::unique_ptr<ILogger> newLogger() override;

        [[nodiscard]] std::unique_ptr<ILogger> newNamedLogger(std::string_view name) override;

        void setLogLevel(::logging::Level level) override;

      private:
        std::shared_ptr<::rust::Box<::logging::console::ConsoleLoggerBuilder>> consoleLoggerBuilder;
    };
}

#endif // VMICORE_CONSOLELOGGERBUILDER_H
