#include "Dumping.h"
#include "Common.h"
#include "Filenames.h"
#include "Scanner.h"
#include <algorithm>
#include <iostream>

using VmiCore::MemoryRegion;
using VmiCore::Plugin::LogLevel;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    Dumping::Dumping(const PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration)
        : pluginInterface(pluginInterface), configuration(std::move(configuration))
    {
        dumpingPath = this->configuration->getOutputPath() / "dumpedRegions";
    }

    void Dumping::dumpMemoryRegion(const std::string& processName,
                                   pid_t pid,
                                   const MemoryRegion& memoryRegionDescriptor,
                                   const std::vector<uint8_t>& data)
    {
        auto memoryRegionInformation =
            createMemoryRegionInformation(processName, pid, memoryRegionDescriptor, getNextRegionId());
        auto inMemDumpFileName = memoryRegionInformation->getMemFileName();

        pluginInterface->logMessage(LogLevel::info,
                                    LOG_FILENAME,
                                    "Dumping Memory region from " + memoryRegionInformation->startAddress +
                                        " with size " + std::to_string(memoryRegionInformation->scanSize) +
                                        " from Process: " + processName + " : " + std::to_string(pid) +
                                        " Module: " + memoryRegionInformation->moduleName + " to " + inMemDumpFileName);

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
        auto startAddress = intToHex(memoryRegionDescriptor.base);
        auto endAddress = intToHex(memoryRegionDescriptor.base + memoryRegionDescriptor.size);

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
