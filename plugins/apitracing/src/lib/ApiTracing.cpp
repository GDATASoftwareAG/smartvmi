#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Filenames.h"
#include "Tracer.h"
#include "config/TracingTargetsParser.h"
#include "os/windows/Library.h"
#include <tclap/CmdLine.h>
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/PluginInit.h>
#include <vmicore/plugins/PluginInterface.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::ILogger;
using VmiCore::OperatingSystem;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    namespace
    {
        PluginInterface* pluginInterface;
        std::shared_ptr<Tracer> tracer;
        std::unique_ptr<ILogger> logger;
    }

    void processStartCallback(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        logger->info("Starting monitoring of process",
                     {{"Process", *processInformation->fullName}, {"Pid", processInformation->pid}});
        tracer->addHooks(*processInformation);
    }

    void shutDownCallback()
    {
        tracer->removeHooks();
    }

    extern "C" bool init(PluginInterface* communicator,
                         std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
                         std::vector<std::string> args)
    {
        pluginInterface = communicator;

        TCLAP::CmdLine cmd("ApiTracing plugin for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::ValueArg functionDefinitionsPath{
            "f",
            "functionDefinitions",
            "Path to the file containing the required definitions for traced functions",
            false,
            std::filesystem::path(CONFIG_DIR) / "functiondefinitions" / "functionDefinitions.yaml",
            "/path/to/functionDefinitions.yaml",
            cmd};
        TCLAP::ValueArg traceProcessName{
            "n", "process", "Process name to trace", false, std::string(""), "process name", cmd};
        cmd.parse(args);

        logger = pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME);
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
        logger->debug("ApiTracing plugin version info", {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});

        auto configPath = config->configFilePath().value_or(std::filesystem::path(CONFIG_DIR) / "configuration.yml");
        TracingTargetsParser tracingTargetsParser(configPath);

        if (traceProcessName.isSet())
        {
            tracingTargetsParser.addTracingTarget(traceProcessName);
        }

        auto functionDefinitions = std::make_shared<FunctionDefinitions>(functionDefinitionsPath);
        functionDefinitions->init();
        switch (pluginInterface->getIntrospectionAPI()->getOsType())
        {
            case OperatingSystem::WINDOWS:
            {
                auto library = std::make_shared<Windows::Library>();
                tracer = std::make_shared<Tracer>(pluginInterface, tracingTargetsParser, functionDefinitions, library);
                break;
            }
            default:
            {
                throw std::runtime_error("Unknown operating system.");
            }
        }

        try
        {
            pluginInterface->registerProcessStartEvent(processStartCallback);
        }
        catch (const std::exception& exc)
        {
            logger->error("Could not register ProcessStart Event", {{"Exception", exc.what()}});
            return false;
        }

        try
        {
            pluginInterface->registerShutdownEvent(shutDownCallback);
        }
        catch (const std::exception& exc)
        {
            logger->error("Could not register ShutDown Event", {{"Exception", exc.what()}});
            return false;
        }

        logger->debug("ApiTracing plugin: init end");

        return true;
    }
}
