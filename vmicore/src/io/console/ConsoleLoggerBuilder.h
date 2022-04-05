#ifndef VMICORE_CONSOLELOGGERBUILDER_H
#define VMICORE_CONSOLELOGGERBUILDER_H

#include "../ILogging.h"

class ConsoleLoggerBuilder : public ILogging
{
  public:
    explicit ConsoleLoggerBuilder(
        std::shared_ptr<::rust::Box<::logging::console::ConsoleLoggerBuilder>> consoleLoggerBuilder);
    ~ConsoleLoggerBuilder() override = default;

    void start() override {}

    void stop(const uint64_t&) override {}

    std::unique_ptr<ILogger> newLogger() override;

    std::unique_ptr<ILogger> newNamedLogger(const std::string& name) override;

    void setLogLevel(::logging::Level level) override;

  private:
    std::shared_ptr<::rust::Box<::logging::console::ConsoleLoggerBuilder>> consoleLoggerBuilder;
};

#endif // VMICORE_CONSOLELOGGERBUILDER_H
