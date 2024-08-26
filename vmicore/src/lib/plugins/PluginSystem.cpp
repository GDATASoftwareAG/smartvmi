#include "PluginSystem.h"
#include "../vmi/MemoryMapping.h"
#include "PluginException.h"
#include <bit>
#include <cstdint>
#include <dlfcn.h>
#include <fmt/core.h>
#include <utility>
#include <vmicore/filename.h>

namespace VmiCore
{
    namespace
    {
        bool isInstanciated = false;
    }

    PluginSystem::PluginSystem(std::shared_ptr<IConfigParser> configInterface,
                               std::shared_ptr<ILibvmiInterface> vmiInterface,
                               std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                               std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor,
                               std::shared_ptr<IFileTransport> pluginLogging,
                               std::shared_ptr<ILogging> loggingLib,
                               std::shared_ptr<IEventStream> eventStream)
        : configInterface(std::move(configInterface)),
          vmiInterface(std::move(vmiInterface)),
          activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
          interruptEventSupervisor(std::move(interruptEventSupervisor)),
          fileTransport(std::move(pluginLogging)),
          loggingLib(std::move(loggingLib)),
          logger(this->loggingLib->newNamedLogger(FILENAME_STEM)),
          eventStream(std::move(eventStream))
    {
        if (isInstanciated)
        {
            throw std::runtime_error("Not allowed to initialize more than one instance of PluginSystem.");
        }
        isInstanciated = true;
    }

    PluginSystem::~PluginSystem()
    {
        isInstanciated = false;
    }

    std::unique_ptr<IMemoryMapping>
    PluginSystem::mapProcessMemoryRegion(addr_t baseVA, addr_t dtb, std::size_t numberOfPages) const
    {
        return std::make_unique<MemoryMapping>(
            loggingLib, vmiInterface, vmiInterface->mmapGuest(baseVA, dtb, numberOfPages));
    }

    void PluginSystem::registerProcessStartEvent(
        const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& startCallback)
    {
        registeredProcessStartCallbacks.push_back(startCallback);
    }

    void PluginSystem::registerProcessTerminationEvent(
        const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& terminationCallback)
    {
        registeredProcessTerminationCallbacks.push_back(terminationCallback);
    }

    std::shared_ptr<IBreakpoint>
    PluginSystem::createBreakpoint(uint64_t targetVA,
                                   const ActiveProcessInformation& processInformation,
                                   const std::function<BpResponse(IInterruptEvent&)>& callbackFunction)
    {
        return interruptEventSupervisor->createBreakpoint(targetVA, processInformation, callbackFunction, false);
    }

    std::unique_ptr<ILogger> PluginSystem::newNamedLogger(std::string_view name) const
    {
        return loggingLib->newNamedLogger(name);
    }

    void PluginSystem::writeToFile(const std::string& filename, const std::string& message) const
    {
        try
        {
            fileTransport->saveBinaryToFile(filename, std::vector<uint8_t>(message.begin(), message.end()));
        }
        catch (const std::exception& e)
        {
            logger->error("Failed to write string to file",
                          {{"filename", filename}, {"message", message}, {"exception", e.what()}});
            eventStream->sendErrorEvent(e.what());
        }
    }

    void PluginSystem::writeToFile(const std::string& filename, const std::vector<uint8_t>& data) const
    {
        try
        {
            fileTransport->saveBinaryToFile(filename, data);
        }
        catch (const std::exception& e)
        {
            logger->error("Failed to write binary to file", {{"filename", filename}, {"exception", e.what()}});
            eventStream->sendErrorEvent(e.what());
        }
    }

    void PluginSystem::sendErrorEvent(std::string_view message) const
    {
        eventStream->sendErrorEvent(message);
    }

    void PluginSystem::sendInMemDetectionEvent(std::string_view message) const
    {
        eventStream->sendInMemDetectionEvent(message);
    }

    std::shared_ptr<IIntrospectionAPI> PluginSystem::getIntrospectionAPI() const
    {
        return vmiInterface;
    }

    std::unique_ptr<std::string> PluginSystem::getResultsDir() const
    {
        return std::make_unique<std::string>(configInterface->getResultsDirectory());
    }

    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
    PluginSystem::getRunningProcesses() const
    {
        return activeProcessesSupervisor->getActiveProcesses();
    }

    void PluginSystem::initializePlugin(const std::string& pluginName,
                                        std::shared_ptr<Plugin::IPluginConfig> config,
                                        const std::vector<std::string>& args)
    {
        auto pluginDirectory = configInterface->getPluginDirectory();
        logger->debug("Plugin directory", {{"dirName", pluginDirectory.string()}});
        auto pluginFilename = pluginDirectory / pluginName;
        logger->debug("Loading plugin", {{"fileName", pluginFilename.string()}});

        void* libraryHandle = dlopen(pluginFilename.c_str(), RTLD_LAZY);
        if (libraryHandle == nullptr)
        {
            throw PluginException(pluginName, fmt::format("Unable to load library: {}", dlerror()));
        }
        dlerror(); // clear possible remnants of error messages

        auto* pluginApiVersion = reinterpret_cast<uint8_t*>(dlsym(libraryHandle, "_ZN7VmiCore6Plugin11API_VERSIONE"));
        auto* dlErrorMessage = dlerror();
        if (dlErrorMessage != nullptr)
        {
            throw PluginException(
                pluginName,
                fmt::format("Unable to retrieve extern symbol '_ZN7VmiCore6Plugin11API_VERSIONE': {}", dlErrorMessage));
        }

        if (*pluginApiVersion != Plugin::PluginInterface::API_VERSION)
        {
            throw PluginException(pluginName,
                                  fmt::format("Plugin API Version {} is incompatible with VmiCore API Version {}",
                                              *pluginApiVersion,
                                              Plugin::PluginInterface::API_VERSION));
        }

        auto pluginInitFunction =
            std::bit_cast<decltype(Plugin::vmicore_plugin_init)*>(dlsym(libraryHandle, PLUGIN_INIT_FUNCTION.data()));
        dlErrorMessage = dlerror();
        if (dlErrorMessage != nullptr)
        {
            throw PluginException(pluginName, fmt::format("Unable to retrieve init function: {}", dlErrorMessage));
        }

        auto plugin = pluginInitFunction(dynamic_cast<Plugin::PluginInterface*>(this), std::move(config), args);

        plugins.emplace_back(pluginName, std::move(plugin));
    }

    void PluginSystem::initializePlugins(const std::map<std::string, std::vector<std::string>, std::less<>>& pluginArgs)
    {
        for (const auto& [name, config] : configInterface->getPlugins())
        {
            initializePlugin(
                name, config, pluginArgs.contains(name) ? pluginArgs.at(name) : std::vector<std::string>{name});
        }
    }

    void PluginSystem::passProcessStartEventToRegisteredPlugins(
        std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        for (const auto& processStartCallback : registeredProcessStartCallbacks)
        {
            processStartCallback(processInformation);
        }
    }

    void PluginSystem::passProcessTerminationEventToRegisteredPlugins(
        std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        for (const auto& processTerminationCallback : registeredProcessTerminationCallbacks)
        {
            processTerminationCallback(processInformation);
        }
    }

    void PluginSystem::unloadPlugins()
    {
        vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
        vmiInterface->flushPageCache();

        for (auto& [name, plugin] : plugins)
        {
            try
            {
                plugin->unload();
            }
            catch (const std::exception& e)
            {
                logger->error("Error occurred while unloading plugin", {{"Plugin", name}, {"Exception", e.what()}});
                eventStream->sendErrorEvent(e.what());
            }
        }

        registeredProcessStartCallbacks.clear();
        registeredProcessTerminationCallbacks.clear();
        plugins.clear();
    }
}
