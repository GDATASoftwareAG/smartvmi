#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Config.h"
#include "Filenames.h"
#include "Tracer.h"
#include "config/TracingTargetsParser.h"
#include "os/windows/Library.h"
#include <fmt/core.h>
#include <tclap/CmdLine.h>
#include <vmicore/plugins/PluginInit.h>
#include <vmicore/plugins/PluginInterface.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::OperatingSystem;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::LogLevel;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    namespace
    {
        PluginInterface* pluginInterface;
        std::shared_ptr<Tracer> tracer;
    }

    void processStartCallback(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        pluginInterface->logMessage(
            LogLevel::info,
            LOG_FILENAME,
            fmt::format("Starting process: {} with pid: {}", *processInformation->fullName, processInformation->pid));
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
        TCLAP::ValueArg<std::filesystem::path> functionDefinitionsPath{
            "f",
            "functionDefinitions",
            "Path to the file containing the required definitions for traced functions",
            false,
            "",
            "/path/to/functionDefinitions.yaml",
            cmd};
        cmd.parse(args);

        pluginInterface->logMessage(LogLevel::debug,
                                    LOG_FILENAME,
                                    "ApiTracing plugin version info: " + std::string(PLUGIN_VERSION) + "-" +
                                        std::string(BUILD_VERSION));

        auto tracingTargetsParser = std::make_shared<TracingTargetsParser>();
        try
        {
            auto configuration = std::make_shared<Config>(config->configFilePath().value());
            auto tracingInformation = tracingTargetsParser->getTracingTargets(configuration->getTracingTargetsPath());
            auto functionDefinitions = std::make_shared<FunctionDefinitions>(functionDefinitionsPath);
            functionDefinitions->init();
            switch (pluginInterface->getIntrospectionAPI()->getOsType())
            {
                case OperatingSystem::WINDOWS:
                {
                    auto library = std::make_shared<Windows::Library>();
                    tracer = std::make_shared<Tracer>(
                        pluginInterface, configuration, tracingInformation, functionDefinitions, library);
                    break;
                }
                default:
                {
                    throw std::runtime_error("Unknown operating system.");
                }
            }
        }
        catch (const ConfigException& exc)
        {
            pluginInterface->logMessage(
                LogLevel::error, LOG_FILENAME, "Error loading configuration: " + std::string(exc.what()));
            return false;
        }

        try
        {
            pluginInterface->registerProcessStartEvent(processStartCallback);
        }
        catch (const std::exception&)
        {
            pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, "Could not register ProcessStart Event");
            return false;
        }

        try
        {
            pluginInterface->registerShutdownEvent(shutDownCallback);
        }
        catch (const std::exception&)
        {
            pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, "Could not register ShutDown Event");
            return false;
        }

        pluginInterface->logMessage(LogLevel::debug, LOG_FILENAME, "apiTracing Plugin: init end");

        return true;
    }
}
