#include "Dumping.h"
#include "Common.h"
#include "Filenames.h"
#include "Scanner.h"
#include <algorithm>
#include <fmt/core.h>

using VmiCore::MemoryRegion;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    Dumping::Dumping(PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration)
        : pluginInterface(pluginInterface),
          configuration(std::move(configuration)),
          logger(pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME))
    {
        dumpingPath = this->configuration->getOutputPath() / "dumpedRegions";
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
    }

    void Dumping::dumpMemoryRegion(const std::string& processName,
                                   pid_t pid,
                                   const MemoryRegion& memoryRegionDescriptor,
                                   const std::vector<uint8_t>& data)
    {
        auto memoryRegionInformation =
            createMemoryRegionInformation(processName, pid, memoryRegionDescriptor, getNextRegionId());
        auto inMemDumpFileName = memoryRegionInformation->getMemFileName();

        logger->info("Dumping Memory region",
                     {{"VA", memoryRegionInformation->startAddress},
                      {"Size", memoryRegionInformation->scanSize},
                      {"Process", processName},
                      {"Pid", pid},
                      {"Module", memoryRegionInformation->moduleName},
                      {"DumpFile", inMemDumpFileName}});

        pluginInterface->writeToFile(dumpingPath / inMemDumpFileName, data);

        auto inMemRegionInfo = memoryRegionInformation->toString();

        appendRegionInfo(inMemRegionInfo);
    }

    std::unique_ptr<MemoryRegionInformation> Dumping::createMemoryRegionInformation(
        const std::string& processName, pid_t pid, const MemoryRegion& memoryRegionDescriptor, int regionId)
    {
        auto moduleName = Scanner::getFilenameFromPath(memoryRegionDescriptor.moduleName);
        if (moduleName->empty())
        {
            *moduleName = "private";
        }

        auto flags = memoryRegionDescriptor.protection->toString();
        auto startAddress = fmt::format("{:x}", memoryRegionDescriptor.base);
        auto endAddress = fmt::format("{:x}", memoryRegionDescriptor.base + memoryRegionDescriptor.size);

        auto memRegionInformation = MemoryRegionInformation{std::to_string(pid),
                                                            memoryRegionDescriptor.size,
                                                            flags,
                                                            memoryRegionDescriptor.isBeingDeleted,
                                                            memoryRegionDescriptor.isProcessBaseImage,
                                                            memoryRegionDescriptor.isSharedMemory,
                                                            *moduleName,
                                                            processName,
                                                            std::to_string(regionId),
                                                            startAddress,
                                                            endAddress};
        auto memRegionInformationUniquePointer = std::make_unique<MemoryRegionInformation>(memRegionInformation);

        return memRegionInformationUniquePointer;
    }

    int Dumping::getNextRegionId()
    {
        std::scoped_lock guard(counterLock);
        return memoryRegionCounter++;
    }

    void Dumping::appendRegionInfo(const std::string& regionInfo)
    {
        std::scoped_lock guard(lock);
        memoryRegionInfo.emplace_back(regionInfo);
    }

    std::vector<std::string> Dumping::getAllMemoryRegionInformation()
    {
        std::scoped_lock guard(lock);
        return memoryRegionInfo;
    }
}
