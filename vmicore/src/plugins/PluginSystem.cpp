#include "PluginSystem.h"
#include "../io/ILogger.h"
#include "../os/PagingDefinitions.h"
#include "../vmi/VmiException.h"
#include <cstdint>
#include <dlfcn.h>
#include <exception>
#include <fmt/core.h>
#include <utility>

#define PLUGIN_NAME "Main"
#define PLUGIN_VERSION "2.0"

namespace
{
    bool isInstanciated = false;
    constexpr char const* paddingLogFile = "memoryExtractionPaddingLog.txt";
}

PluginSystem::PluginSystem(std::shared_ptr<IConfigParser> configInterface,
                           std::shared_ptr<ILibvmiInterface> vmiInterface,
                           std::shared_ptr<ActiveProcessesSupervisor> activeProcessesSupervisor,
                           std::shared_ptr<IFileTransport> pluginLogging,
                           std::shared_ptr<ILogging> loggingLib,
                           std::shared_ptr<IEventStream> eventStream)
    : configInterface(std::move(configInterface)),
      vmiInterface(std::move(vmiInterface)),
      activeProcessesSupervisor(std::move(activeProcessesSupervisor)),
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
PluginSystem::readProcessMemoryRegion(pid_t pid, Plugin::virtual_address_t address, size_t count) const
{
    if (count % PagingDefinitions::pageSizeInBytes != 0)
    {
        throw std::invalid_argument("Size of memory region must be page size aligned.");
    }
    auto numberOfPages = count >> PagingDefinitions::numberOfPageIndexBits;
    auto process = activeProcessesSupervisor->getProcessInformationByPid(pid);
    return readPagesWithUnmappedRegionPadding(address, process->processCR3, numberOfPages);
}

std::unique_ptr<std::vector<Plugin::MemoryRegion>> PluginSystem::getProcessMemoryRegions(pid_t pid) const
{
    logger->debug("Called for process",
                  {logfield::create("ProcessId", static_cast<int64_t>(pid)),
                   logfield::create("Function", static_cast<std::string>(__func__))});

    auto processInformation = activeProcessesSupervisor->getProcessInformationByPid(pid);
    auto vadtList = processInformation->vadTree->getAllVadts();
    logger->debug("Vadt", {logfield::create("count", static_cast<uint64_t>(vadtList->size()))});

    std::unique_ptr<std::vector<Plugin::MemoryRegion>> memoryRegionsVector;
    if (!vadtList->empty())
    {
        memoryRegionsVector = std::make_unique<std::vector<Plugin::MemoryRegion>>();

        for (const auto& vadtListElement : *vadtList)
        {
            auto startAddress = vadtListElement.startingVPN << PagingDefinitions::numberOfPageIndexBits;
            auto endAddress = ((vadtListElement.endingVPN + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
            auto size = endAddress - startAddress + 1;
            logger->debug("Vadt element",
                          {logfield::create("startingVPN", fmt::format("{:#x}", vadtListElement.startingVPN)),
                           logfield::create("endingVPN", fmt::format("{:#x}", vadtListElement.endingVPN)),
                           logfield::create("startAddress", fmt::format("{:#x}", startAddress)),
                           logfield::create("endAddress", fmt::format("{:#x}", endAddress)),
                           logfield::create("size", static_cast<uint64_t>(size))});
            memoryRegionsVector->emplace_back(startAddress,
                                              size,
                                              vadtListElement.fileName,
                                              vadtListElement.protection,
                                              vadtListElement.isSharedMemory,
                                              vadtListElement.isBeingDeleted,
                                              vadtListElement.isProcessBaseImage);
        }
    }
    else
    {
        memoryRegionsVector = std::make_unique<std::vector<Plugin::MemoryRegion>>();
    }
    return memoryRegionsVector;
}

void PluginSystem::registerProcessTerminationEvent(Plugin::processTerminationCallback_f terminationCallback)
{
    registeredProcessTerminationCallbacks.push_back(terminationCallback);
}

void PluginSystem::registerShutdownEvent(Plugin::shutdownCallback_f shutdownCallback)
{
    registeredShutdownCallbacks.push_back(shutdownCallback);
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

void PluginSystem::logMessage(Plugin::LogLevel logLevel, const std::string& filename, const std::string& message) const
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

void PluginSystem::sendErrorEvent(const std::string& message) const
{
    eventStream->sendErrorEvent(message);
}

void PluginSystem::sendInMemDetectionEvent(const std::string& message) const
{
    eventStream->sendInMemDetectionEvent(message);
}

std::unique_ptr<std::string> PluginSystem::getResultsDir() const
{
    return std::make_unique<std::string>(configInterface->getResultsDirectory());
}

std::unique_ptr<std::vector<Plugin::ProcessInformation>> PluginSystem::getRunningProcesses() const
{
    auto activeProcesses = activeProcessesSupervisor->getActiveProcesses();
    auto activeProcessesForPlugin = std::make_unique<std::vector<Plugin::ProcessInformation>>();
    for (const auto& activeProcess : *activeProcesses)
    {
        activeProcessesForPlugin->emplace_back(activeProcess->pid, *activeProcess->fullName);
    }
    return activeProcessesForPlugin;
}

void PluginSystem::initializePlugin(const std::string& pluginName, std::shared_ptr<Plugin::IPluginConfig> config)
{
    auto pluginDirectory = configInterface->getPluginDirectory();
    logger->debug("Plugin directory",
                  {
                      logfield::create("dirName", pluginDirectory.string()),
                  });
    auto pluginFilename = pluginDirectory / pluginName;
    logger->debug("Loading plugin",
                  {
                      logfield::create("fileName", pluginFilename),
                  });

    void* libraryHandle = dlopen(pluginFilename.c_str(), RTLD_LAZY);
    if (libraryHandle == nullptr)
    {
        throw std::runtime_error("Unable to load library: " + std::string(dlerror()));
    }
    dlerror(); // clear possible remnants of error messages

    auto* pluginInformation = reinterpret_cast<Plugin::PluginDetails*>(dlsym(libraryHandle, "pluginInformation"));
    auto* dlErrorMessage = dlerror();
    if (dlErrorMessage != nullptr)
    {
        throw std::runtime_error("Unable to retrieve extern symbol 'pluginInformation': " +
                                 std::string(dlErrorMessage));
    }
    logger->debug("Plugin information",
                  {
                      logfield::create("API", static_cast<int64_t>(pluginInformation->apiVersion)),
                      logfield::create("Pluginname", pluginInformation->pluginName),
                      logfield::create("PluginVersion", pluginInformation->pluginVersion),
                  });

    if (pluginInformation->apiVersion != VMI_PLUGIN_API_VERSION)
    {
        throw std::runtime_error("Plugin API Version " + std::to_string(pluginInformation->apiVersion) +
                                 " is incompatible with VmiCore API Version " + std::to_string(VMI_PLUGIN_API_VERSION));
    }

    auto pluginInitFunction = reinterpret_cast<Plugin::init_f>(dlsym(libraryHandle, "init"));
    dlErrorMessage = dlerror();
    if (dlErrorMessage != nullptr)
    {
        throw std::runtime_error("Unable to retrieve function 'init': " + std::string(dlErrorMessage));
    }

    if (!pluginInitFunction(dynamic_cast<Plugin::PluginInterface*>(this), std::move(config)))
    {
        throw std::runtime_error("Unable to initialize plugin " + pluginName);
    }
}

void PluginSystem::passProcessTerminationEventToRegisteredPlugins(pid_t pid, const std::string& processName)
{
    for (auto& processTerminationCallback : registeredProcessTerminationCallbacks)
    {
        processTerminationCallback(pid, processName.c_str());
    }
}

void PluginSystem::passShutdownEventToRegisteredPlugins()
{
    for (auto& shutdownCallback : registeredShutdownCallbacks)
    {
        shutdownCallback();
    }
}
