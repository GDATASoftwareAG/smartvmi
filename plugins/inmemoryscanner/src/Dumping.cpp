#include "Dumping.h"
#include "Common.h"
#include "Filenames.h"
#include "Scanner.h"
#include <algorithm>
#include <iostream>

Dumping::Dumping(const Plugin::PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration)
    : pluginInterface(pluginInterface), configuration(std::move(configuration))
{
    inMemoryDumpingFolder = *this->pluginInterface->getResultsDir();
    inMemoryDumpingFolder /= this->configuration->getOutputPath();
    dumpingPath = inMemoryDumpingFolder / "dumpedRegions";
}

void Dumping::dumpMemoryRegion(const std::string& processName,
                               pid_t pid,
                               const Plugin::MemoryRegion& memoryRegionDescriptor,
                               const std::vector<uint8_t>& data)
{
    auto memoryRegionInformation =
        createMemoryRegionInformation(processName, pid, memoryRegionDescriptor, getNextRegionId());
    auto inMemDumpFileName = memoryRegionInformation->getMemFileName();

    pluginInterface->logMessage(LogLevel::info,
                                LOG_FILENAME,
                                "Dumping Memory region from " + memoryRegionInformation->startAdress + " with size " +
                                    std::to_string(memoryRegionInformation->scanSize) +
                                    " from Process: " + processName + " : " + std::to_string(pid) +
                                    " Module: " + memoryRegionInformation->moduleName + " to " + inMemDumpFileName);

    pluginInterface->writeToFile(dumpingPath / inMemDumpFileName, data);

    auto inMemRegionInfo = memoryRegionInformation->toString();

    appendRegionInfo(inMemRegionInfo);
}

std::unique_ptr<std::string> Dumping::protectionToString(ProtectionValues protection)
{
    auto protectionAsString = std::make_unique<std::string>();
    switch (protection)
    {
        case ProtectionValues::PAGE_NOACCESS:
        case ProtectionValues::PAGE_NOACCESS_2:
        case ProtectionValues::PAGE_NOACCESS_3:
        case ProtectionValues::PAGE_NOACCESS_4:
            protectionAsString->append("N");
            break;
        case ProtectionValues::PAGE_READONLY:
        case ProtectionValues::PAGE_NOCACHE_PAGE_READONLY:
        case ProtectionValues::PAGE_GUARD_PAGE_READONLY:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_READONLY:
            protectionAsString->append("R");
            break;
        case ProtectionValues::PAGE_READWRITE:
        case ProtectionValues::PAGE_NOCACHE_PAGE_READWRITE:
        case ProtectionValues::PAGE_GUARD_PAGE_READWRITE:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_READWRITE:
            protectionAsString->append("RW");
            break;
        case ProtectionValues::PAGE_EXECUTE:
        case ProtectionValues::PAGE_NOCACHE_PAGE_EXECUTE:
        case ProtectionValues::PAGE_GUARD_PAGE_EXECUTE:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_EXECUTE:
            protectionAsString->append("X");
            break;
        case ProtectionValues::PAGE_EXECUTE_READ:
        case ProtectionValues::PAGE_NOCACHE_PAGE_EXECUTE_READ:
        case ProtectionValues::PAGE_GUARD_PAGE_EXECUTE_READ:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_EXECUTE_READ:
            protectionAsString->append("RX");
            break;
        case ProtectionValues::PAGE_EXECUTE_READWRITE:
        case ProtectionValues::PAGE_NOCACHE_PAGE_EXECUTE_READWRITE:
        case ProtectionValues::PAGE_GUARD_PAGE_EXECUTE_READWRITE:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_EXECUTE_READWRITE:
            protectionAsString->append("RWX");
            break;
        case ProtectionValues::PAGE_WRITECOPY:
        case ProtectionValues::PAGE_NOCACHE_PAGE_WRITECOPY:
        case ProtectionValues::PAGE_GUARD_PAGE_WRITECOPY:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_WRITECOPY:
            protectionAsString->append("WC");
            break;
        case ProtectionValues::PAGE_EXECUTE_WRITECOPY:
        case ProtectionValues::PAGE_NOCACHE_PAGE_EXECUTE_WRITECOPY:
        case ProtectionValues::PAGE_GUARD_PAGE_EXECUTE_WRITECOPY:
        case ProtectionValues::PAGE_WRITECOMBINE_PAGE_EXECUTE_WRITECOPY:
            protectionAsString->append("WCX");
            break;
    }
    return protectionAsString;
}

std::unique_ptr<MemoryRegionInformation> Dumping::createMemoryRegionInformation(
    const std::string& processName, pid_t pid, const Plugin::MemoryRegion& memoryRegionDescriptor, int regionId)
{
    auto moduleName = Scanner::getFilenameFromPath(memoryRegionDescriptor.moduleName);
    if (moduleName->empty())
    {
        *moduleName = "private";
    }

    auto flags = protectionToString(memoryRegionDescriptor.protection);
    auto startAdress = intToHex(memoryRegionDescriptor.baseAddress);
    auto endAdress = intToHex(memoryRegionDescriptor.baseAddress + memoryRegionDescriptor.size);

    auto memRegionInformation = MemoryRegionInformation{std::to_string(pid),
                                                        memoryRegionDescriptor.size,
                                                        *flags,
                                                        memoryRegionDescriptor.isBeingDeleted,
                                                        memoryRegionDescriptor.isProcessBaseImage,
                                                        memoryRegionDescriptor.isSharedMemory,
                                                        *moduleName,
                                                        processName,
                                                        std::to_string(regionId),
                                                        startAdress,
                                                        endAdress};
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
