#include "Scanner.h"
#include "Common.h"
#include "Filenames.h"
#include <fmt/core.h>
#include <future>
#include <iterator>

using VmiCore::ActiveProcessInformation;
using VmiCore::addr_t;
using VmiCore::MemoryRegion;
using VmiCore::pid_t;
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
          dumping(std::move(dumping)),
          logger(this->pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME)),
          inMemResultsLogger(this->pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
        inMemResultsLogger->bind(
            {{VmiCore::WRITE_TO_FILE_TAG, (this->configuration->getOutputPath() / TEXT_RESULT_FILENAME).string()}});
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
            logger->info("Skipping: Is shared memory and not the process base image.");
        }
        return verdict;
    }

    void
    Scanner::scanMemoryRegion(pid_t pid, const std::string& processName, const MemoryRegion& memoryRegionDescriptor)
    {
        logger->info("Scanning Memory region",
                     {{"VA", fmt::format("{:x}", memoryRegionDescriptor.base)},
                      {"Size", memoryRegionDescriptor.size},
                      {"Module", memoryRegionDescriptor.moduleName}});

        if (shouldRegionBeScanned(memoryRegionDescriptor))
        {
            auto scanSize = memoryRegionDescriptor.size;
            auto maximumScanSize = configuration->getMaximumScanSize();
            if (scanSize > maximumScanSize)
            {
                logger->info("Memory region is too big, reducing to maximum scan size",
                             {{"Size", scanSize}, {"MaximumScanSize", maximumScanSize}});
                scanSize = maximumScanSize;
            }

            logger->debug("Start getProcessMemoryRegion", {{"Size", scanSize}});

            auto memoryRegion = pluginInterface->readProcessMemoryRegion(pid, memoryRegionDescriptor.base, scanSize);

            logger->debug("End getProcessMemoryRegion", {{"Size", scanSize}});
            if (memoryRegion->empty())
            {
                logger->debug("Extracted memory region has size 0, skipping");
            }
            else
            {
                if (configuration->isDumpingMemoryActivated())
                {
                    logger->debug("Start dumpVadRegionToFile", {{"Size", memoryRegion->size()}});
                    dumping->dumpMemoryRegion(processName, pid, memoryRegionDescriptor, *memoryRegion);
                    logger->debug("End dumpVadRegionToFile");
                }

                logger->debug("Start scanMemory", {{"Size", memoryRegion->size()}});

                // The semaphore protects the yara rules from being accessed more than YR_MAX_THREADS (32 atm.) times in
                // parallel.
                semaphore.wait();
                auto results = yaraEngine->scanMemory(*memoryRegion);
                semaphore.notify();

                logger->debug("End scanMemory");

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
                        scanMemoryRegion(
                            processInformation->pid, *processInformation->fullName, memoryRegionDescriptor);
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
