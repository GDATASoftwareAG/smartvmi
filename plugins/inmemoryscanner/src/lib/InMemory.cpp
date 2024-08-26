#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "InMemory.h"
#include "Common.h"
#include "Config.h"
#include "Dumping.h"
#include "Filenames.h"
#include "YaraInterface.h"
#include <memory>
#include <string>
#include <tclap/CmdLine.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::IPlugin;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    InMemory::InMemory(PluginInterface* pluginInterface, const IPluginConfig& config, std::vector<std::string>& args)
        : pluginInterface(pluginInterface), logger(pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        TCLAP::CmdLine cmd("InMemory scanner plugin for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::SwitchArg dumpMemoryArgument("d", "dump", "Dump memory.", cmd);

        cmd.parse(args);

        logger->debug("InMemoryScanner plugin version info",
                      {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});

        std::shared_ptr<IConfig> configuration = std::make_shared<Config>(pluginInterface);
        configuration->parseConfiguration(config);
        if (dumpMemoryArgument.isSet())
        {
            configuration->overrideDumpMemoryFlag(dumpMemoryArgument.getValue());
        }
        auto yara = std::make_unique<YaraInterface>(configuration->getSignatureFile(), configuration->getScanTimeout());
        auto dumping = std::make_unique<Dumping>(pluginInterface, configuration);
        scanner = std::make_unique<Scanner>(pluginInterface, configuration, std::move(yara), std::move(dumping));
    }

    void InMemory::unload()
    {
        logger->info("Shutdown initiated");
        try
        {
            scanner->scanAllProcesses();
        }
        catch (const std::exception& exc)
        {
            logger->error("Error occurred during shutdown scan", {{"Exception", exc.what()}});
            pluginInterface->sendErrorEvent(exc.what());
        }
        scanner->saveOutput();
        logger->info("Done scanning all processes");
    }
}

extern "C" std::unique_ptr<IPlugin> VmiCore::Plugin::vmicore_plugin_init(
    PluginInterface* pluginInterface,
    std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
    std::vector<std::string> args)
{
    return std::make_unique<InMemoryScanner::InMemory>(pluginInterface, *config, args);
}
