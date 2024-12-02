#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.0.0"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#include "StringFinder.h"
#include "Filenames.h"
#include <algorithm>
#include <tclap/CmdLine.h>
#include <vmicore/callback.h>

VMI_PLUGIN_API_VERSION_INFO

using VmiCore::ActiveProcessInformation;
using VmiCore::ILogger;
using VmiCore::Plugin::IPlugin;
using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

namespace StringFinder
{
    StringFinder::StringFinder(PluginInterface* pluginInterface,
                               const IPluginConfig& _config,
                               std::vector<std::string>& args)
        : pluginInterface(pluginInterface), logger(pluginInterface->newNamedLogger(STRINGFINDER_LOGGER_NAME))
    {
        TCLAP::CmdLine cmd("String Finder plugin for VMICore.", ' ', PLUGIN_VERSION);
        TCLAP::ValueArg searchStringArg{
            "s", "search", "String to search for in memory", false, std::string(""), "search string", cmd};
        cmd.parse(args);

        searchString = {searchStringArg.getValue().begin(), searchStringArg.getValue().end()};
    }

    void StringFinder::findInMemoryString(std::shared_ptr<const ActiveProcessInformation> processInfo)
    {
        if (processInfo->name != "notepad.exe")
        {
            return;
        }

        logger->info("START SCANNING MEMORY", {{"Process", processInfo->name}});

        auto mapping = pluginInterface->mapProcessMemoryRegion(0,
                                                               processInfo->processUserDtb,
                                                               VmiCore::PagingDefinitions::kernelspaceLowerBoundary /
                                                                   VmiCore::PagingDefinitions::pageSizeInBytes);

        for (const auto& region : mapping->getMappedRegions())
        {
            auto span = region.asSpan();
            std::span u16span(reinterpret_cast<const char16_t*>(span.data()), span.size() / 2);

            auto searchResult = std::search(u16span.begin(), u16span.end(), searchString.begin(), searchString.end());
            if (searchResult != u16span.begin() && searchResult != u16span.end())
            {
                logger->info("STRING FOUND!",
                             {{"Process", processInfo->name},
                              {"String", std::string(searchString.begin(), searchString.end())}});
            }
        }
    }

    void StringFinder::unload()
    {
        auto processes = pluginInterface->getRunningProcesses();
        for (const auto& process : *processes)
        {
            findInMemoryString(process);
        }
    }
}

extern "C" std::unique_ptr<IPlugin> VmiCore::Plugin::vmicore_plugin_init(PluginInterface* pluginInterface,
                                                                         std::shared_ptr<IPluginConfig> config,
                                                                         // NOLINT(performance-unnecessary-value-param)
                                                                         std::vector<std::string> args)
{
    return std::make_unique<StringFinder::StringFinder>(pluginInterface, *config, args);
}