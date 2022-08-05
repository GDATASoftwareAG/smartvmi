#include "Scanner.h"
#include "Filenames.h"
#include <future>
#include <iterator>

Scanner::Scanner(const Plugin::PluginInterface* pluginInterface,
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
            Plugin::LogLevel::info, LOG_FILENAME, "Skipping: Is shared memory and not the process base image.");
    }
    return verdict;
}

void Scanner::scanMemoryRegion(pid_t pid, const std::string& processName, const MemoryRegion& memoryRegionDescriptor)
{
    pluginInterface->logMessage(Plugin::LogLevel::info,
                                LOG_FILENAME,
                                "Scanning Memory region from " + intToHex(memoryRegionDescriptor.base) + " with size " +
                                    intToHex(memoryRegionDescriptor.size) + " name " +
                                    memoryRegionDescriptor.moduleName);

    if (shouldRegionBeScanned(memoryRegionDescriptor))
    {
        auto scanSize = memoryRegionDescriptor.size;
        auto maximumScanSize = configuration->getMaximumScanSize();
        if (scanSize > maximumScanSize)
        {
            pluginInterface->logMessage(Plugin::LogLevel::info,
                                        LOG_FILENAME,
                                        "Memory region is too big, reduce to " + intToHex(maximumScanSize));
            scanSize = maximumScanSize;
        }

        pluginInterface->logMessage(
            Plugin::LogLevel::debug, LOG_FILENAME, "Start getProcessMemoryRegion with size: " + intToHex(scanSize));

        auto memoryRegion = pluginInterface->readProcessMemoryRegion(pid, memoryRegionDescriptor.base, scanSize);

        pluginInterface->logMessage(Plugin::LogLevel::debug,
                                    LOG_FILENAME,
                                    "End getProcessMemoryRegion with size: " + intToHex(memoryRegion->size()));
        if (memoryRegion->empty())
        {
            pluginInterface->logMessage(
                Plugin::LogLevel::debug, LOG_FILENAME, "Extracted memory region has size 0, skipping");
        }
        else
        {
            if (configuration->isDumpingMemoryActivated())
            {
                pluginInterface->logMessage(Plugin::LogLevel::debug,
                                            LOG_FILENAME,
                                            "Start dumpVadRegionToFile with size: " + intToHex(memoryRegion->size()));
                dumping->dumpMemoryRegion(processName, pid, memoryRegionDescriptor, *memoryRegion);
                pluginInterface->logMessage(Plugin::LogLevel::debug, LOG_FILENAME, "End dumpVadRegionToFile");
            }

            pluginInterface->logMessage(
                Plugin::LogLevel::debug, LOG_FILENAME, "Start scanMemory with size: " + intToHex(memoryRegion->size()));

            // The semaphore protects the yara rules from being accessed more than YR_MAX_THREADS (32 atm.) times in
            // parallel.
            semaphore.wait();
            auto results = yaraEngine->scanMemory(*memoryRegion);
            semaphore.notify();

            pluginInterface->logMessage(Plugin::LogLevel::debug, LOG_FILENAME, "End scanMemory");

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

void Scanner::scanProcess(pid_t pid, const std::string& processName)
{
    if (pid == 0)
    {
        throw std::invalid_argument("Scanning pid 0 should never happen");
    }
    if (configuration->isProcessIgnored(processName))
    {
        pluginInterface->logMessage(Plugin::LogLevel::info,
                                    LOG_FILENAME,
                                    "Process " + std::to_string(pid) + " \"" + processName +
                                        "\" is ignored due to process name");
    }
    else
    {
        pluginInterface->logMessage(Plugin::LogLevel::info,
                                    LOG_FILENAME,
                                    "Scanning process " + std::to_string(pid) + " \"" + processName + "\"");
        try
        {
            auto memoryRegions = pluginInterface->getProcessMemoryRegions(pid);

            for (const auto& memoryRegionDescriptor : *memoryRegions)
            {
                try
                {
                    scanMemoryRegion(pid, processName, memoryRegionDescriptor);
                }
                catch (const std::exception& exc)
                {
                    auto errorMsg =
                        "Error scanning memory region of process " + processName + ": " + std::string(exc.what());
                    pluginInterface->logMessage(Plugin::LogLevel::error, LOG_FILENAME, errorMsg);
                    pluginInterface->sendErrorEvent(errorMsg);
                }
            }
        }
        catch (const std::exception& exc)
        {
            auto errorMsg = "Error scanning process " + processName + ": " + std::string(exc.what());
            pluginInterface->logMessage(Plugin::LogLevel::error, LOG_FILENAME, errorMsg);
            pluginInterface->sendErrorEvent(errorMsg);
        }
        pluginInterface->logMessage(Plugin::LogLevel::info,
                                    LOG_FILENAME,
                                    "Done scanning process " + std::to_string(pid) + " \"" + processName + "\"");
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
            scanProcessAsyncTasks.push_back(
                std::async(&Scanner::scanProcess, this, process->pid, (*process->fullName)));
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
                                          Plugin::virtual_address_t base,
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
        pluginInterface->logMessage(Plugin::LogLevel::info, inMemoryResultsTextFile, matchesAsString);
    }
}
