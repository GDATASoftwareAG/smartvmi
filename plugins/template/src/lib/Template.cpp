#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Filenames.h"
#include "TemplateCode.h"
#include <tclap/CmdLine.h>
#include <vmicore/plugins/PluginInit.h>

// Ensure API compatibility between plugin and VmiCore. When loading a plugin the API version against which it was
// compiled is checked in VmiCore.
VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::ILogger;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

// This file contains the main interface between the VmiCore and the plugin. Here resides the init-function as well as
// the callbacks for every event.
namespace Template
{

    namespace
    {
        std::unique_ptr<ILogger> logger;
        PluginInterface* pluginInterface;
        std::shared_ptr<TemplateCode> templateCode;
    }

    // As an example when a new process is started this callback is called. Check the PluginInterface.h for additional
    // available events.
    void processStartCallback(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        logger->info("Process started", {{"Process", *processInformation->fullName}, {"Pid", processInformation->pid}});
        templateCode->doStuffWithProcessStart(*processInformation);
    }

    // This is the init function. It gets called by the VmiCore and is responsible for setting up logging, command line
    // parameters and other required functionality.
    extern "C" bool init(PluginInterface* communicator,
                         std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
                         std::vector<std::string> args)
    {
        pluginInterface = communicator;

        // Set up command line parameter with TCLAP as needed.
        TCLAP::CmdLine cmd("Plugin Template for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::ValueArg parameter1{"p",
                                   "parameter1",
                                   "Placeholder for a parameter",
                                   false,
                                   std::string("parameter1 default value"),
                                   "type description for parameter1",
                                   cmd};
        cmd.parse(args);

        logger = pluginInterface->newNamedLogger(TEMPLATE_LOGGER_NAME);
        logger->debug("Template plugin version info", {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});

        if (parameter1.isSet())
        {
            logger->debug("Got command line parameter", {{"parameter1", parameter1.getValue()}});
        }

        // Create the objects which contain the logic of the plugin
        templateCode = std::make_shared<TemplateCode>(pluginInterface, pluginInterface->getIntrospectionAPI());

        // Register required events
        try
        {
            pluginInterface->registerProcessStartEvent(processStartCallback);
        }
        catch (const std::exception& exc)
        {
            logger->error("Could not register ProcessStart Event", {{"Exception", exc.what()}});
            return false;
        }

        logger->debug("Template plugin: init end");

        return true;
    }
}
