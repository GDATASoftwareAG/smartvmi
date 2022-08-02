#include "Tracer.h"
#include "Config.h"
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
                loadedModules.insert({*filename, memoryRegionDescriptor.baseAddress});
            }
        }
    }
}

std::unique_ptr<std::string> Tracer::getFilenameFromPath(const std::string& path)
{
    auto filename = std::make_unique<std::string>(path);
    auto pos = path.rfind('\\');
    if (pos != std::string::npos)
    {
        filename = std::make_unique<std::string>(path, pos + 1);
    }
    return filename;
}

void Tracer::injectHooks(std::vector<std::string> hookList)
{
    for (auto& hook : hookList)
    {
        for (auto& module : loadedModules)
        {
            if (module.first == hook)
            {
                printf("foo");
            }
        }
    }
}

std::map<std::string, uint64_t> Tracer::getLoadedModules()
{
    return loadedModules;
}
