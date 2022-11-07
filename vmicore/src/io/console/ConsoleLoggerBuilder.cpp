#include "ConsoleLoggerBuilder.h"
#include "ConsoleLogger.h"
#include <memory>

namespace VmiCore
{
    ConsoleLoggerBuilder::ConsoleLoggerBuilder(
        std::shared_ptr<::rust::Box<::logging::console::ConsoleLoggerBuilder>> consoleLoggerBuilder)
        : consoleLoggerBuilder(std::move(consoleLoggerBuilder))
    {
    }

    std::unique_ptr<ILogger> ConsoleLoggerBuilder::newLogger()
    {
        return std::make_unique<ConsoleLogger>((*consoleLoggerBuilder)->new_logger());
    }

    std::unique_ptr<ILogger> ConsoleLoggerBuilder::newNamedLogger(const std::string_view& name)
    {
        return std::make_unique<ConsoleLogger>(
            (*consoleLoggerBuilder)->new_named_logger(static_cast<std::string>(name)));
    }

    void ConsoleLoggerBuilder::setLogLevel(::logging::Level level)
    {
        (*consoleLoggerBuilder)->set_log_level(level);
    }
}
