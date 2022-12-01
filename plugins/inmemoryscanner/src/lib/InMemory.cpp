#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Config.h"
#include "Dumping.h"
#include "Filenames.h"
#include "Scanner.h"
#include "Yara.h"
#include <memory>
#include <string>
#include <tclap/CmdLine.h>
#include <vmicore/plugins/PluginInit.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::LogLevel;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    PluginInterface* pluginInterface;
    std::unique_ptr<Scanner> scanner;

    void shutdownCallback()
    {
        pluginInterface->logMessage(LogLevel::info, LOG_FILENAME, "Shutdown initiated");
        try
        {
            scanner->scanAllProcesses();
        }
        catch (const std::exception& exc)
        {
            auto errorMsg = "Error occurred during shutdown scan. Reason: " + std::string(exc.what());
            pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, errorMsg);
            pluginInterface->sendErrorEvent(errorMsg);
        }
        scanner->saveOutput();
        pluginInterface->logMessage(LogLevel::info, LOG_FILENAME, "Done scanning all processes");
    }

    void processTerminationCallback(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        scanner->scanProcess(std::move(processInformation));
    }

    extern "C" bool init(PluginInterface* communicator,
                         std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
                         std::vector<std::string> args)
    {
        auto success = true;
        pluginInterface = communicator;
        TCLAP::CmdLine cmd("InMemory scanner plugin for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::SwitchArg dumpMemoryArgument("d", "dump", "Dump memory.", cmd);

        cmd.parse(args);

        pluginInterface->logMessage(LogLevel::debug,
                                    LOG_FILENAME,
                                    "InMemoryScanner plugin version info: " + std::string(PLUGIN_VERSION) + "-" +
                                        std::string(BUILD_VERSION));

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
                LogLevel::error, LOG_FILENAME, "Error loading configuration: " + std::string(exc.what()));
            success = false;
        }
        catch (const YaraException& exc)
        {
            pluginInterface->logMessage(
                LogLevel::error, LOG_FILENAME, "Error loading yara: " + std::string(exc.what()));
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
                    LogLevel::error, LOG_FILENAME, "Could not register ProcessTermination Event");
                success = false;
            }
            try
            {
                pluginInterface->registerShutdownEvent(shutdownCallback);
            }
            catch (const std::exception&)
            {
                pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, "Could not register Shutdown Event");
                success = false;
            }
            pluginInterface->logMessage(LogLevel::info, LOG_FILENAME, "inMemory Plugin: init end");
        }

        return success;
    }
}