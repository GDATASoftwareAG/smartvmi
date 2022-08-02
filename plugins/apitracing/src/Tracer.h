#include "Config.h"
#include <PluginInterface.h>
#include <map>
#include <memory>

#ifndef VMICORE_TRACER_H
#define VMICORE_TRACER_H

class Tracer
{
  public:
    Tracer(const Plugin::PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration);
    void initLoadedModules(pid_t pid);
    void injectHooks(std::vector<std::string>);
    std::map<std::string, uint64_t> getLoadedModules();

  private:
    const Plugin::PluginInterface* pluginInterface;
    std::shared_ptr<IConfig> configuration;
    std::map<std::string, uint64_t> loadedModules;

    std::unique_ptr<std::string> getFilenameFromPath(const std::string& path);
};

#endif // VMICORE_TRACER_H
