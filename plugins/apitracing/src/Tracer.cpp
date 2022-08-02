#include "Tracer.h"
#include "Config.h"
#include "Filenames.h"

#include <fmt/core.h>
#include <algorithm>
#include <filesystem>
#include <vector>

Tracer::Tracer(const Plugin::PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration)
    : pluginInterface(pluginInterface), configuration(std::move(configuration))

{
}

void Tracer::initLoadedModules(pid_t pid)
{
    auto memoryRegions = pluginInterface->getProcessMemoryRegions(pid);

    for (const auto& memoryRegionDescriptor : *memoryRegions)
    {
        auto filename = getFilenameFromPath(memoryRegionDescriptor.moduleName);

        if (loadedModules.find(*filename) == loadedModules.end())
        {
            if (std::filesystem::path(*filename).extension() == ".dll")
            {
                loadedModules.insert({*filename, memoryRegionDescriptor.baseAddress}); //TODO check if virtual or physical
            }
        }
    }
}

void Tracer::addHooks(pid_t pid, std::shared_ptr<std::string> processName)
{
    if (!shouldProcessBeMonitored(pid, *processName))
    {
        return;
    }

    tracedProcesses.insert({pid, processName});
    initLoadedModules(pid);
    try
    {
        injectHooks(*processName);
    }
    catch (const std::exception& e)
    {
        tracedProcesses.erase(pid);
        pluginInterface->logMessage(Plugin::LogLevel::warning,LOG_FILENAME,fmt::format("Could not trace process {} with pid {} because {}", *processName, pid, e.what()));
    }
}


std::unique_ptr<std::string> Tracer::getFilenameFromPath(const std::string& path) const
{
    auto filename = std::make_unique<std::string>(path);
    auto pos = path.rfind('\\');
    if (pos != std::string::npos)
    {
        filename = std::make_unique<std::string>(path, pos + 1);
    }
    return filename;
}

void Tracer::injectHooks(const std::string& processName) const
{
    auto hookTargets = configuration->getHookTargets(processName);
    for (auto& module : loadedModules)
    {
        auto moduleHookTargets = hookTargets.at(module.first);
        for (auto moduleHookTarget : moduleHookTargets)
        {
            hookFunction(module.first, moduleHookTarget);
        }
    }
}

std::map<std::string, uint64_t> Tracer::getLoadedModules()
{
    return loadedModules;
}

bool Tracer::shouldProcessBeMonitored(pid_t pid, const std::string& processName) const
{
    if((processName == configuration->getInitialProcessName()) || (tracedProcesses.find(pid) != tracedProcesses.end()))
    {
        return true;
    }
    return false;
}

void Tracer::hookFunction(const std::string& moduleName, const std::string& functionName) const
{
    //TODO
    return;
}
