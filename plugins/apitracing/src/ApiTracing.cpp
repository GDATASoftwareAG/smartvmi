#include "Config.h"
#include "Filenames.h"
#include "Tracer.h"
#include <PluginInterface.h>
#include <fmt/core.h>

// Struct is externally verified during plugin initialization
[[maybe_unused]] extern constexpr Plugin::PluginDetails pluginInformation = {
    VMI_PLUGIN_API_VERSION, PLUGIN_NAME, PLUGIN_VERSION};

Plugin::PluginInterface* pluginInterface;
std::unique_ptr<Tracer> tracer;

void processStartCallback(pid_t processID, std::shared_ptr<std::string> processName)
{
    pluginInterface->logMessage(Plugin::LogLevel::info,
                                LOG_FILENAME,
                                fmt::format("Starting process: {} with pid: {}", processName->c_str(), processID));
    tracer->addHooks(processID, processName);
}

extern "C" bool init(Plugin::PluginInterface* communicator, std::shared_ptr<Plugin::IPluginConfig> config)
{
    auto success = true;
    pluginInterface = communicator;

    pluginInterface->logMessage(
        Plugin::LogLevel::info, LOG_FILENAME, "Starting apiTracing plugin build version " + std::string(BUILD_VERSION));

    try
    {
        std::shared_ptr<IConfig> configuration = std::make_shared<Config>(pluginInterface);
        configuration->parseConfiguration(*config);
        tracer = std::make_unique<Tracer>(pluginInterface, configuration);
    }
    catch (const ConfigException& exc)
    {
        pluginInterface->logMessage(
            Plugin::LogLevel::error, LOG_FILENAME, "Error loading configuration: " + std::string(exc.what()));
        success = false;
    }

    if (success)
    {
        try
        {
            pluginInterface->registerProcessStartEvent(processStartCallback);
        }
        catch (const std::exception&)
        {
            pluginInterface->logMessage(Plugin::LogLevel::error, LOG_FILENAME, "Could not register ProcessStart Event");
            success = false;
        }

        pluginInterface->logMessage(Plugin::LogLevel::info, LOG_FILENAME, "apiTracing Plugin: init end");
    }

    return success;
}
