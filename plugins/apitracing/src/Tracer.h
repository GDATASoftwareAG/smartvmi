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
    std::map<std::string, uint64_t> getLoadedModules();
    void addHooks(pid_t pid, std::shared_ptr<std::string> processName);

  private:
    const Plugin::PluginInterface* pluginInterface;
    std::shared_ptr<IConfig> configuration;
    std::map<std::string, uint64_t> loadedModules;
    std::map<pid_t, std::shared_ptr<std::string>> tracedProcesses;

    void injectHooks(const std::string& processName)const;
    std::unique_ptr<std::string> getFilenameFromPath(const std::string& path) const;
    bool shouldProcessBeMonitored(pid_t pid, const std::string& processName) const;
    void hookFunction(const std::string& moduleName, const std::string& functionName) const;
};

#endif // VMICORE_TRACER_H
