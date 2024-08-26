#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "Template.h"
#include "Filenames.h"
#include <tclap/CmdLine.h>

// Ensure API compatibility between plugin and VmiCore. When loading a plugin the API version against which it was
// compiled is checked in VmiCore.
VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::ILogger;
using VmiCore::Plugin::IPlugin;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

// This file contains the main interface between the VmiCore and the plugin. Here resides the init function.
namespace Template
{
    Template::Template(VmiCore::Plugin::PluginInterface* pluginInterface,
                       const VmiCore::Plugin::IPluginConfig& config,
                       std::vector<std::string>& args)
        : logger(pluginInterface->newNamedLogger(TEMPLATE_LOGGER_NAME))
    {
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

        logger->debug("Template plugin version info", {{"Version", PLUGIN_VERSION}, {"BuildNumber", BUILD_VERSION}});

        if (parameter1.isSet())
        {
            logger->debug("Got command line parameter", {{"parameter1", parameter1.getValue()}});
        }

        // Create the objects which contain the logic of the plugin
        templateCode = std::make_unique<TemplateCode>(pluginInterface, pluginInterface->getIntrospectionAPI());
    }
}

// This is the init function. It is linked and called dynamically at runtime by
// VmiCore and is responsible for creating an instance of a plugin.
// All plugins have to inherit from IPlugin.
extern "C" std::unique_ptr<IPlugin> VmiCore::Plugin::vmicore_plugin_init(
    PluginInterface* pluginInterface,
    std::shared_ptr<IPluginConfig> config, // NOLINT(performance-unnecessary-value-param)
    std::vector<std::string> args)
{
    return std::make_unique<Template::Template>(pluginInterface, *config, args);
}
