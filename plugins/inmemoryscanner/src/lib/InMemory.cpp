#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Common.h"
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
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    PluginInterface* pluginInterface;
    std::unique_ptr<Scanner> scanner;
    std::unique_ptr<VmiCore::ILogger> staticLogger;

    void shutdownCallback()
    {
        staticLogger->info("Shutdown initiated");
        try
        {
            scanner->scanAllProcesses();
        }
        catch (const std::exception& exc)
        {
            staticLogger->error("Error occurred during shutdown scan", {{"Exception", exc.what()}});
            pluginInterface->sendErrorEvent(exc.what());
        }
        scanner->saveOutput();
        staticLogger->info("Done scanning all processes");
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

        staticLogger = communicator->newNamedLogger(INMEMORY_LOGGER_NAME);
        staticLogger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
        staticLogger->debug("InMemoryScanner plugin version info",
                            {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});
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
            staticLogger->error("Error loading configuration", {{"Exception", exc.what()}});
            success = false;
        }
        catch (const YaraException& exc)
        {
            staticLogger->error("Error loading yara", {{"Exception", exc.what()}});
            success = false;
        }

        if (success)
        {
            try
            {
                pluginInterface->registerProcessTerminationEvent(processTerminationCallback);
            }
            catch (const std::exception& exc)
            {
                staticLogger->error("Could not register ProcessTermination Event", {{"Exception", exc.what()}});
                success = false;
            }
            try
            {
                pluginInterface->registerShutdownEvent(shutdownCallback);
            }
            catch (const std::exception& exc)
            {
                staticLogger->error("Could not register Shutdown Event", {{"Exception", exc.what()}});
                success = false;
            }
            staticLogger->info("InMemory Plugin: init end");
        }

        return success;
    }
}
