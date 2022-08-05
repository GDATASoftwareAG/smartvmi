#include "Config.h"
#include "Dumping.h"
#include "Filenames.h"
#include "Scanner.h"
#include "Yara.h"
#include <PluginInit.h>
#include <memory>
#include <string>
#include <tclap/CmdLine.h>

// Struct is externally verified during plugin initialization
[[maybe_unused]] extern constexpr Plugin::PluginDetails pluginInformation = {
    VMI_PLUGIN_API_VERSION, PLUGIN_NAME, PLUGIN_VERSION};

Plugin::PluginInterface* pluginInterface;
std::unique_ptr<Scanner> scanner;

void shutdownCallback()
{
    pluginInterface->logMessage(Plugin::LogLevel::info, LOG_FILENAME, "Shutdown initiated");
    try
    {
        scanner->scanAllProcesses();
    }
    catch (const std::exception& exc)
    {
        auto errorMsg = "Error occurred during shutdown scan. Reason: " + std::string(exc.what());
        pluginInterface->logMessage(Plugin::LogLevel::error, LOG_FILENAME, errorMsg);
        pluginInterface->sendErrorEvent(errorMsg);
    }
    scanner->saveOutput();
    pluginInterface->logMessage(Plugin::LogLevel::info, LOG_FILENAME, "Done scanning all processes");
}

void processTerminationCallback(std::shared_ptr<const ActiveProcessInformation> processInformation)
{
    scanner->scanProcess(std::move(processInformation));
}

extern "C" bool init(Plugin::PluginInterface* communicator,
                     std::shared_ptr<Plugin::IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
                     std::vector<std::string> args)
{
    auto success = true;
    pluginInterface = communicator;
    TCLAP::CmdLine cmd("InMemory scanner plugin for VMICore.", ' ', PLUGIN_VERSION);
    TCLAP::SwitchArg dumpMemoryArgument("d", "dump", "Dump memory.", cmd);

    cmd.parse(args);

    pluginInterface->logMessage(
        Plugin::LogLevel::info, LOG_FILENAME, "Starting inMemory plugin build version " + std::string(BUILD_VERSION));

    try
    {
        std::shared_ptr<IConfig> configuration = std::make_shared<Config>(pluginInterface);
        configuration->parseConfiguration(*config);
        if (dumpMemoryArgument.isSet())
        {
            configuration->overrideDumpMemoryFlag(dumpMemoryArgument.getValue());
        }
        auto yara = std::make_unique<Yara>(configuration->getSignatureFile());
        auto dumping = std::make_unique<Dumping>(pluginInterface, configuration);
        scanner = std::make_unique<Scanner>(pluginInterface, configuration, std::move(yara), std::move(dumping));
    }
    catch (const ConfigException& exc)
    {
        pluginInterface->logMessage(
            Plugin::LogLevel::error, LOG_FILENAME, "Error loading configuration: " + std::string(exc.what()));
        success = false;
    }
    catch (const YaraException& exc)
    {
        pluginInterface->logMessage(
            Plugin::LogLevel::error, LOG_FILENAME, "Error loading yara: " + std::string(exc.what()));
        success = false;
    }

    if (success)
    {
        try
        {
            pluginInterface->registerProcessTerminationEvent(processTerminationCallback);
        }
        catch (const std::exception&)
        {
            pluginInterface->logMessage(
                Plugin::LogLevel::error, LOG_FILENAME, "Could not register ProcessTermination Event");
            success = false;
        }
        try
        {
            pluginInterface->registerShutdownEvent(shutdownCallback);
        }
        catch (const std::exception&)
        {
            pluginInterface->logMessage(Plugin::LogLevel::error, LOG_FILENAME, "Could not register Shutdown Event");
            success = false;
        }
        pluginInterface->logMessage(Plugin::LogLevel::info, LOG_FILENAME, "inMemory Plugin: init end");
    }

    return success;
}
