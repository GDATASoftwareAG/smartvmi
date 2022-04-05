#ifndef VMICORE_CONSOLELOGGER_H
#define VMICORE_CONSOLELOGGER_H

#include "../ILogger.h"

class ConsoleLogger : public ILogger
{
  public:
    explicit ConsoleLogger(::rust::Box<::logging::console::ConsoleLogger> logger);
    ~ConsoleLogger() override = default;

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
    void warning(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

    inline void error(std::string message) const override
    {
        error(message, {});
    };
    void error(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const override;

  private:
    ::rust::Box<::logging::console::ConsoleLogger> logger;
};

#endif // VMICORE_CONSOLELOGGER_H
