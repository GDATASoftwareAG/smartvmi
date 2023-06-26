#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "ApiTracing.h"
#include "Filenames.h"
#include "config/Config.h"
#include "os/windows/Library.h"
#include <tclap/CmdLine.h>
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/PluginInterface.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::OperatingSystem;
using VmiCore::Plugin::IPlugin;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    ApiTracing::ApiTracing(VmiCore::Plugin::PluginInterface* pluginInterface,
                           const VmiCore::Plugin::IPluginConfig& config,
                           std::vector<std::string>& args)
        : logger(pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        TCLAP::CmdLine cmd("ApiTracing plugin for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::ValueArg functionDefinitionsPath{
            "f",
            "functionDefinitions",
            "Path to the file containing the required definitions for traced functions",
            false,
            std::filesystem::path("."),
            "/path/to/functionDefinitions.yaml",
            cmd};
        TCLAP::ValueArg traceProcessName{
            "n", "process", "Process name to trace", false, std::string(""), "process name", cmd};
        cmd.parse(args);

        logger->debug("ApiTracing plugin version info", {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});

        auto apiTracingConfig = std::make_unique<Config>(config);

        if (functionDefinitionsPath.isSet())
        {
            apiTracingConfig->setFunctionDefinitionsPath(functionDefinitionsPath);
        }
        if (traceProcessName.isSet())
        {
            apiTracingConfig->addTracingTarget(traceProcessName);
        }

        auto functionDefinitions =
            std::make_shared<FunctionDefinitions>(apiTracingConfig->getFunctionDefinitionsPath());
        functionDefinitions->init();

        switch (pluginInterface->getIntrospectionAPI()->getOsType())
        {
            case OperatingSystem::WINDOWS:
            {
                auto library = std::make_shared<Windows::Library>();
                auto tracedProcessFactory =
                    std::make_shared<TracedProcessFactory>(pluginInterface, functionDefinitions, library);
                tracer = std::make_shared<Tracer>(pluginInterface, std::move(apiTracingConfig), tracedProcessFactory);
                break;
            }
            default:
            {
                throw std::runtime_error("Unknown operating system.");
            }
        }
    }

    void ApiTracing::unload()
    {
        tracer->teardown();
    }
}

std::unique_ptr<IPlugin>
VmiCore::Plugin::init(PluginInterface* pluginInterface,
                      std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
                      std::vector<std::string> args)
{
    return std::make_unique<ApiTracing::ApiTracing>(pluginInterface, *config, args);
}
