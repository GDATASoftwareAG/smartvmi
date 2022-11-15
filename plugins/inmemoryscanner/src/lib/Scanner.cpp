#include "Scanner.h"
#include "Common.h"
#include "Filenames.h"
#include <future>
#include <iterator>
#include <span>
#include <vmicore/os/PagingDefinitions.h>

using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MappedRegion;
using VmiCore::MemoryRegion;
using VmiCore::pid_t;
using VmiCore::PagingDefinitions::pageSizeInBytes;
using VmiCore::Plugin::LogLevel;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    Scanner::Scanner(const PluginInterface* pluginInterface,
                     std::shared_ptr<IConfig> configuration,
                     std::unique_ptr<YaraInterface> yaraEngine,
                     std::unique_ptr<IDumping> dumping)
        : pluginInterface(pluginInterface),
          configuration(std::move(configuration)),
          yaraEngine(std::move(yaraEngine)),
          dumping(std::move(dumping))
    {
        inMemoryResultsTextFile = this->configuration->getOutputPath() / TEXT_RESULT_FILENAME;
    }

    std::unique_ptr<std::string> Scanner::getFilenameFromPath(const std::string& path)
    {
        auto filename = std::make_unique<std::string>(path);
        auto pos = path.rfind('\\');
        if (pos != std::string::npos)
        {
            filename = std::make_unique<std::string>(path, pos + 1);
        }
        return filename;
    }

    bool Scanner::shouldRegionBeScanned(const MemoryRegion& memoryRegionDescriptor)
    {
        bool verdict = true;
        if (configuration->isScanAllRegionsActivated())
        {
            return true;
        }
        if (memoryRegionDescriptor.isSharedMemory && !memoryRegionDescriptor.isProcessBaseImage)
        {
            verdict = false;
            pluginInterface->logMessage(
                LogLevel::info, LOG_FILENAME, "Skipping: Is shared memory and not the process base image.");
        }
        return verdict;
    }

    std::vector<uint8_t> Scanner::constructPaddedMemoryRegion(const std::vector<MappedRegion>& regions)
    {
        std::vector<uint8_t> result;

        if (regions.empty())
        {
            return result;
        }

        std::size_t regionSize = 0;
        for (const auto& region : regions)
        {
            regionSize += region.mapping.size();
            regionSize += pageSizeInBytes;
        }
        // last region should not have succeeding padding page
        regionSize -= pageSizeInBytes;

        result.reserve(regionSize);
        // copy first region
        std::copy(regions.front().mapping.begin(), regions.front().mapping.end(), std::back_inserter(result));

        for (std::size_t i = 1; i < regions.size(); i++)
        {
            const auto& region = regions[i];
            // padding page
            result.insert(result.end(), pageSizeInBytes, 0);
            std::copy(region.mapping.begin(), region.mapping.end(), std::back_inserter(result));
        }

        return result;
    }

    void Scanner::scanMemoryRegion(pid_t pid,
                                   addr_t dtb,
                                   const std::string& processName,
                                   const MemoryRegion& memoryRegionDescriptor)
    {
        pluginInterface->logMessage(LogLevel::info,
                                    LOG_FILENAME,
                                    "Scanning Memory region from " + intToHex(memoryRegionDescriptor.base) +
                                        " with size " + intToHex(memoryRegionDescriptor.size) + " name " +
                                        memoryRegionDescriptor.moduleName);

        if (shouldRegionBeScanned(memoryRegionDescriptor))
        {
            auto memoryMapping = pluginInterface->mapProcessMemoryRegion(
                memoryRegionDescriptor.base, dtb, bytesToNumberOfPages(memoryRegionDescriptor.size));
            auto mappedRegions = memoryMapping->getMappedRegions().lock();

            if (mappedRegions->empty())
            {
                pluginInterface->logMessage(
                    LogLevel::debug, LOG_FILENAME, "Extracted memory region has size 0, skipping");
            }
            else
            {
                if (configuration->isDumpingMemoryActivated())
                {
                    pluginInterface->logMessage(LogLevel::debug,
                                                LOG_FILENAME,
                                                "Start dumpVadRegionToFile with size: " +
                                                    intToHex(memoryMapping->getSizeInGuest()));

                    auto paddedRegion = constructPaddedMemoryRegion(*mappedRegions);

                    dumping->dumpMemoryRegion(processName, pid, memoryRegionDescriptor, paddedRegion);
                    pluginInterface->logMessage(LogLevel::debug, LOG_FILENAME, "End dumpVadRegionToFile");
                }

                pluginInterface->logMessage(LogLevel::debug,
                                            LOG_FILENAME,
                                            "Start scanMemory with size: " + intToHex(memoryMapping->getSizeInGuest()));

                // The semaphore protects the yara rules from being accessed more than YR_MAX_THREADS (32 atm.) times in
                // parallel.
                semaphore.wait();
                auto results = yaraEngine->scanMemory(*mappedRegions);
                semaphore.notify();

                pluginInterface->logMessage(LogLevel::debug, LOG_FILENAME, "End scanMemory");

                if (!results->empty())
                {
                    for (const auto& result : *results)
                    {
                        pluginInterface->sendInMemDetectionEvent(result.ruleName);
                    }
                    outputXml.addResult(processName, pid, memoryRegionDescriptor.base, *results);
                    logInMemoryResultToTextFile(processName, pid, memoryRegionDescriptor.base, *results);
                }
            }
        }
    }

    void Scanner::scanProcess(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        if (processInformation->pid == 0)
        {
            throw std::invalid_argument("Scanning pid 0 should never happen");
        }
        if (configuration->isProcessIgnored(*processInformation->fullName))
        {
            pluginInterface->logMessage(LogLevel::info,
                                        LOG_FILENAME,
                                        "Process " + std::to_string(processInformation->pid) + " \"" +
                                            *processInformation->fullName + "\" is ignored due to process name");
        }
        else
        {
            pluginInterface->logMessage(LogLevel::info,
                                        LOG_FILENAME,
                                        "Scanning process " + std::to_string(processInformation->pid) + " \"" +
                                            *processInformation->fullName + "\"");
            try
            {
                auto memoryRegions = processInformation->memoryRegionExtractor->extractAllMemoryRegions();

                for (const auto& memoryRegionDescriptor : *memoryRegions)
                {
                    try
                    {
                        scanMemoryRegion(processInformation->pid,
                                         processInformation->processCR3,
                                         *processInformation->fullName,
                                         memoryRegionDescriptor);
                    }
                    catch (const std::exception& exc)
                    {
                        auto errorMsg = "Error scanning memory region of process " + *processInformation->fullName +
                                        ": " + std::string(exc.what());
                        pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, errorMsg);
                        pluginInterface->sendErrorEvent(errorMsg);
                    }
                }
            }
            catch (const std::exception& exc)
            {
                auto errorMsg =
                    "Error scanning process " + *processInformation->fullName + ": " + std::string(exc.what());
                pluginInterface->logMessage(LogLevel::error, LOG_FILENAME, errorMsg);
                pluginInterface->sendErrorEvent(errorMsg);
            }
            pluginInterface->logMessage(LogLevel::info,
                                        LOG_FILENAME,
                                        "Done scanning process " + std::to_string(processInformation->pid) + " \"" +
                                            *processInformation->fullName + "\"");
        }
    }

    void Scanner::scanAllProcesses()
    {
        auto processes = pluginInterface->getRunningProcesses();
        std::vector<std::future<void>> scanProcessAsyncTasks;
        for (const auto& process : *processes)
        {
            if (process->pid != 0)
            {
                scanProcessAsyncTasks.push_back(std::async(&Scanner::scanProcess, this, process));
            }
        }
        for (auto& currentTask : scanProcessAsyncTasks)
        {
            currentTask.get();
        }
    }

    void Scanner::saveOutput()
    {
        if (configuration->isDumpingMemoryActivated())
        {
            auto memoryRegionInformation = dumping->getAllMemoryRegionInformation();
            std::ostringstream vts;

            if (!memoryRegionInformation.empty())
            {
                std::copy(memoryRegionInformation.begin(),
                          memoryRegionInformation.end(),
                          std::ostream_iterator<std::string>(vts, "\n"));
            }
            pluginInterface->writeToFile(configuration->getOutputPath() /= "MemoryRegionInformation.json", vts.str());
        }
        pluginInterface->writeToFile(configuration->getOutputPath() /= XML_RESULT_FILENAME, *outputXml.getString());
    }

    void Scanner::logInMemoryResultToTextFile(const std::string& processName,
                                              pid_t pid,
                                              addr_t base,
                                              const std::vector<Rule>& results)
    {
        for (const auto& rule : results)
        {
            std::string matchesAsString(rule.ruleNamespace + " : " + rule.ruleName + " in " + processName + " (" +
                                        std::to_string(pid) + ") at " + intToHex(base) + " matches (");
            for (const auto& match : rule.matches)
            {
                matchesAsString += match.matchName + ", ";
            }
            matchesAsString += ")\n";
            pluginInterface->logMessage(LogLevel::info, inMemoryResultsTextFile, matchesAsString);
        }
    }
}
