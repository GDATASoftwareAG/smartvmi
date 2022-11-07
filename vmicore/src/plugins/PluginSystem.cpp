#include "PluginSystem.h"
#include "../os/PagingDefinitions.h"
#include <cstdint>
#include <dlfcn.h>
#include <exception>
#include <fmt/core.h>
#include <utility>

namespace VmiCore
{
    namespace
    {
        bool isInstanciated = false;
        constexpr char const* paddingLogFile = "memoryExtractionPaddingLog.txt";
    }

    // Must match init function declared in PluginInit.h
    using init_f = bool (*)(Plugin::PluginInterface* pluginInterface,
                            std::shared_ptr<Plugin::IPluginConfig> config,
                            std::vector<std::string> args);

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
          legacyLogging(std::move(pluginLogging)),
          loggingLib(std::move(loggingLib)),
          logger(NEW_LOGGER(this->loggingLib)),
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

    std::unique_ptr<std::vector<uint8_t>>
    PluginSystem::readPagesWithUnmappedRegionPadding(uint64_t pageAlignedVA, uint64_t cr3, uint64_t numberOfPages) const
    {
        if (pageAlignedVA % PagingDefinitions::pageSizeInBytes != 0)
        {
            throw std::invalid_argument(
                fmt::format("{}: Starting address {:#x} is not aligned to page boundary", __func__, pageAlignedVA));
        }
        auto vadIdentifier(fmt::format("CR3 {:#x} VAD @ {:#x}-{:#x}",
                                       cr3,
                                       pageAlignedVA,
                                       (pageAlignedVA + numberOfPages * PagingDefinitions::pageSizeInBytes)));
        auto memoryRegion = std::make_unique<std::vector<uint8_t>>();
        auto needsPadding = true;
        for (uint64_t currentPageIndex = 0; currentPageIndex < numberOfPages; currentPageIndex++)
        {
            auto memoryPage = std::vector<uint8_t>(PagingDefinitions::pageSizeInBytes);
            if (vmiInterface->readXVA(pageAlignedVA, cr3, memoryPage))
            {
                if (!needsPadding)
                {
                    needsPadding = true;
                    logger->info("First successful page extraction after padding",
                                 {logfield::create(WRITE_TO_FILE_TAG, paddingLogFile),
                                  logfield::create("vadIdentifier", vadIdentifier),
                                  logfield::create("pageAlignedVA", fmt::format("{:#x}", pageAlignedVA))});
                }
                memoryRegion->insert(memoryRegion->cend(), memoryPage.cbegin(), memoryPage.cend());
            }
            else
            {
                if (needsPadding)
                {
                    memoryRegion->insert(memoryRegion->cend(), PagingDefinitions::pageSizeInBytes, 0x0);
                    needsPadding = false;
                    logger->info("Start of padding",
                                 {logfield::create(WRITE_TO_FILE_TAG, paddingLogFile),
                                  logfield::create("vadIdentifier", vadIdentifier),
                                  logfield::create("pageAlignedVA", fmt::format("{:#x}", pageAlignedVA))});
                }
            }
            pageAlignedVA += PagingDefinitions::pageSizeInBytes;
        }
        return memoryRegion;
    }

    std::unique_ptr<std::vector<uint8_t>>
    PluginSystem::readProcessMemoryRegion(pid_t pid, addr_t address, size_t count) const
    {
        if (count % PagingDefinitions::pageSizeInBytes != 0)
        {
            throw std::invalid_argument("Size of memory region must be page size aligned.");
        }
        auto numberOfPages = count >> PagingDefinitions::numberOfPageIndexBits;
        auto process = activeProcessesSupervisor->getProcessInformationByPid(pid);
        return readPagesWithUnmappedRegionPadding(address, process->processCR3, numberOfPages);
    }

    void PluginSystem::registerProcessStartEvent(Plugin::processStartCallback_f startCallback)
    {
        registeredProcessStartCallbacks.push_back(startCallback);
    }

    void PluginSystem::registerProcessTerminationEvent(Plugin::processTerminationCallback_f terminationCallback)
    {
        registeredProcessTerminationCallbacks.push_back(terminationCallback);
    }

    void PluginSystem::registerShutdownEvent(Plugin::shutdownCallback_f shutdownCallback)
    {
        registeredShutdownCallbacks.push_back(shutdownCallback);
    }

    std::shared_ptr<IBreakpoint> PluginSystem::createBreakpoint(
        uint64_t targetVA, uint64_t processDtb, const std::function<BpResponse(IInterruptEvent&)>& callbackFunction)
    {
        return interruptEventSupervisor->createBreakpoint(targetVA, processDtb, callbackFunction);
    }

    void PluginSystem::writeToFile(const std::string& filename, const std::string& message) const
    {
        try
        {
            legacyLogging->saveBinaryToFile(filename, std::vector<uint8_t>(message.begin(), message.end()));
        }
        catch (const std::exception& e)
        {
            logger->error("Failed to write string to file",
                          {logfield::create("filename", filename),
                           logfield::create("message", message),
                           logfield::create("exception", e.what())});
            eventStream->sendErrorEvent(e.what());
        }
    }

    void PluginSystem::writeToFile(const std::string& filename, const std::vector<uint8_t>& data) const
    {
        try
        {
            legacyLogging->saveBinaryToFile(filename, data);
        }
        catch (const std::exception& e)
        {
            logger->error("Failed to write binary to file",
                          {logfield::create("filename", filename), logfield::create("exception", e.what())});
            eventStream->sendErrorEvent(e.what());
        }
    }

    void
    PluginSystem::logMessage(Plugin::LogLevel logLevel, const std::string& filename, const std::string& message) const
    {
        switch (logLevel)
        {
            case Plugin::LogLevel::debug:
                logger->debug(message, {logfield::create(WRITE_TO_FILE_TAG, filename)});
                break;
            case Plugin::LogLevel::info:
                logger->info(message, {logfield::create(WRITE_TO_FILE_TAG, filename)});
                break;
            case Plugin::LogLevel::warning:
                logger->warning(message, {logfield::create(WRITE_TO_FILE_TAG, filename)});
                break;
            case Plugin::LogLevel::error:
                logger->error(message, {logfield::create(WRITE_TO_FILE_TAG, filename)});
                break;
            default:
                throw std::invalid_argument("Log level not implemented.");
        }
    }

    void PluginSystem::sendErrorEvent(const std::string_view& message) const
    {
        eventStream->sendErrorEvent(message);
    }

    void PluginSystem::sendInMemDetectionEvent(const std::string_view& message) const
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
        logger->debug("Plugin directory",
                      {
                          logfield::create("dirName", pluginDirectory.string()),
                      });
        auto pluginFilename = pluginDirectory / pluginName;
        logger->debug("Loading plugin",
                      {
                          logfield::create("fileName", pluginFilename.string()),
                      });

        void* libraryHandle = dlopen(pluginFilename.c_str(), RTLD_LAZY);
        if (libraryHandle == nullptr)
        {
            throw std::runtime_error("Unable to load library: " + std::string(dlerror()));
        }
        dlerror(); // clear possible remnants of error messages

        auto* pluginApiVersion = reinterpret_cast<uint8_t*>(dlsym(libraryHandle, "_ZN7VmiCore6Plugin11API_VERSIONE"));
        auto* dlErrorMessage = dlerror();
        if (dlErrorMessage != nullptr)
        {
            throw std::runtime_error("Unable to retrieve extern symbol '_ZN7VmiCore6Plugin11API_VERSIONE': " +
                                     std::string(dlErrorMessage));
        }
        logger->debug("Plugin information", {logfield::create("API_VERSION", static_cast<int64_t>(*pluginApiVersion))});

        if (*pluginApiVersion != Plugin::PluginInterface::API_VERSION)
        {
            throw std::runtime_error("Plugin API Version " + std::to_string(*pluginApiVersion) +
                                     " is incompatible with VmiCore API Version " +
                                     std::to_string(Plugin::PluginInterface::API_VERSION));
        }

        auto pluginInitFunction = reinterpret_cast<init_f>(dlsym(libraryHandle, "init"));
        dlErrorMessage = dlerror();
        if (dlErrorMessage != nullptr)
        {
            throw std::runtime_error("Unable to retrieve function 'init': " + std::string(dlErrorMessage));
        }

        if (!pluginInitFunction(dynamic_cast<Plugin::PluginInterface*>(this), std::move(config), args))
        {
            throw std::runtime_error("Unable to initialize plugin " + pluginName);
        }
    }

    void PluginSystem::passProcessStartEventToRegisteredPlugins(
        std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        for (auto& processStartCallback : registeredProcessStartCallbacks)
        {
            processStartCallback(processInformation);
        }
    }

    void PluginSystem::passProcessTerminationEventToRegisteredPlugins(
        std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        for (auto& processTerminationCallback : registeredProcessTerminationCallbacks)
        {
            processTerminationCallback(processInformation);
        }
    }

    void PluginSystem::passShutdownEventToRegisteredPlugins()
    {
        vmiInterface->flushV2PCache(LibvmiInterface::flushAllPTs);
        vmiInterface->flushPageCache();

        for (auto& shutdownCallback : registeredShutdownCallbacks)
        {
            shutdownCallback();
        }
    }
}
