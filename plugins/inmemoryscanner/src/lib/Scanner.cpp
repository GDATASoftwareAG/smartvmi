#include "Scanner.h"
#include "Common.h"
#include "Filenames.h"
#include <algorithm>
#include <fmt/core.h>
#include <future>
#include <vmicore/callback.h>
#include <vmicore/os/PagingDefinitions.h>

using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MappedRegion;
using VmiCore::MemoryRegion;
using VmiCore::pid_t;
using VmiCore::PagingDefinitions::pageSizeInBytes;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    Scanner::Scanner(PluginInterface* pluginInterface,
                     std::shared_ptr<IConfig> configuration,
                     std::unique_ptr<IYaraInterface> yaraInterface,
                     std::unique_ptr<IDumping> dumping)
        : pluginInterface(pluginInterface),
          configuration(std::move(configuration)),
          yaraInterface(std::move(yaraInterface)),
          dumping(std::move(dumping)),
          logger(pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME)),
          inMemResultsLogger(pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
        inMemResultsLogger->bind(
            {{VmiCore::WRITE_TO_FILE_TAG, (this->configuration->getOutputPath() / TEXT_RESULT_FILENAME).string()}});

        pluginInterface->registerProcessTerminationEvent(VMICORE_SETUP_MEMBER_CALLBACK(scanProcess));
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
        if (configuration->isScanAllRegionsActivated())
        {
            return true;
        }
        if (memoryRegionDescriptor.isSharedMemory && !memoryRegionDescriptor.isProcessBaseImage)
        {
            logger->info("Skipping: Is shared memory and not the process base image.");
            return false;
        }
        return true;
    }

    std::vector<uint8_t> Scanner::constructPaddedMemoryRegion(std::span<const MappedRegion> regions)
    {
        std::vector<uint8_t> result;

        if (regions.empty())
        {
            return result;
        }

        std::size_t regionSize = 0;
        for (const auto& region : regions)
        {
            regionSize += region.asSpan().size();
            regionSize += pageSizeInBytes;
        }
        // last region should not have succeeding padding page
        regionSize -= pageSizeInBytes;

        result.reserve(regionSize);
        // copy first region
        auto frontRegionSpan = regions.front().asSpan();
        std::ranges::copy(frontRegionSpan.begin(), frontRegionSpan.end(), std::back_inserter(result));

        // copy the rest of the regions with a padding page in between each chunk
        for (std::size_t i = 1; i < regions.size(); i++)
        {
            result.insert(result.end(), pageSizeInBytes, 0);
            auto regionSpan = regions[i].asSpan();
            std::ranges::copy(regionSpan.begin(), regionSpan.end(), std::back_inserter(result));
        }

        return result;
    }

    void Scanner::scanMemoryRegion(pid_t pid,
                                   addr_t dtb,
                                   const std::string& processName,
                                   const MemoryRegion& memoryRegionDescriptor)
    {
        logger->info("Scanning Memory region",
                     {{"VA", fmt::format("{:x}", memoryRegionDescriptor.base)},
                      {"Size", memoryRegionDescriptor.size},
                      {"Module", memoryRegionDescriptor.moduleName}});

        if (!shouldRegionBeScanned(memoryRegionDescriptor))
        {
            return;
        }

        auto memoryMapping = pluginInterface->mapProcessMemoryRegion(
            memoryRegionDescriptor.base, dtb, bytesToNumberOfPages(memoryRegionDescriptor.size));
        auto mappedRegions = memoryMapping->getMappedRegions();

        if (mappedRegions.empty())
        {
            logger->debug("Extracted memory region has size 0, skipping");
            return;
        }

        if (configuration->isDumpingMemoryActivated())
        {
            logger->debug("Start dumpVadRegionToFile", {{"Size", memoryRegionDescriptor.size}});

            auto paddedRegion = constructPaddedMemoryRegion(mappedRegions);

            dumping->dumpMemoryRegion(processName, pid, memoryRegionDescriptor, paddedRegion);
        }

        logger->debug("Start scanMemory", {{"Size", memoryRegionDescriptor.size}});

        // The semaphore protects the yara rules from being accessed more than YR_MAX_THREADS (32 atm.) times in
        // parallel.
        semaphore.acquire();
        auto results = yaraInterface->scanMemory(mappedRegions);
        semaphore.release();

        logger->debug("End scanMemory");

        if (!results.empty())
        {
            for (const auto& result : results)
            {
                pluginInterface->sendInMemDetectionEvent(result.ruleName);
            }
            outputXml.addResult(processName, pid, memoryRegionDescriptor.base, results);
            logInMemoryResultToTextFile(processName, pid, memoryRegionDescriptor.base, results);
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
            logger->info("Process is ignored due to process name",
                         {{"Pid", processInformation->pid}, {"Name", *processInformation->fullName}});
        }
        else
        {
            logger->info("Scanning process",
                         {{"Pid", processInformation->pid}, {"Name", *processInformation->fullName}});
            try
            {
                auto memoryRegions = processInformation->memoryRegionExtractor->extractAllMemoryRegions();

                for (const auto& memoryRegionDescriptor : *memoryRegions)
                {
                    try
                    {
                        scanMemoryRegion(processInformation->pid,
                                         processInformation->processUserDtb,
                                         *processInformation->fullName,
                                         memoryRegionDescriptor);
                    }
                    catch (const YaraTimeoutException&)
                    {
                        logger->warning("Scan timeout reached",
                                        {{"Process", *processInformation->fullName},
                                         {"BaseVA", memoryRegionDescriptor.base},
                                         {"Size", memoryRegionDescriptor.size}});
                    }
                    catch (const std::exception& exc)
                    {
                        logger->error("Error scanning memory region of process",
                                      {{"Name", *processInformation->fullName}, {"Exception", exc.what()}});
                        pluginInterface->sendErrorEvent(exc.what());
                    }
                }
            }
            catch (const std::exception& exc)
            {
                logger->error("Error scanning process",
                              {{"Name", *processInformation->fullName}, {"Exception", exc.what()}});
                pluginInterface->sendErrorEvent(exc.what());
            }
            logger->info("Done scanning process",
                         {{"Pid", processInformation->pid}, {"Name", *processInformation->fullName}});
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
            for (const auto& match : rule.matches)
            {
                inMemResultsLogger->info("Rule matches",
                                         {{"Namespace", rule.ruleNamespace},
                                          {"Name", rule.ruleName},
                                          {"Match", match.matchName},
                                          {"Process", processName},
                                          {"Pid", pid},
                                          {"VA", fmt::format("{:x}", base)}});
            }
        }
    }
}
