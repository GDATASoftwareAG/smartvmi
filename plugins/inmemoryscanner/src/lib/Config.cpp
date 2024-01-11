#include "Config.h"
#include "Common.h"
#include "Filenames.h"
#include <algorithm>

using VmiCore::Plugin::IPluginConfig;
using VmiCore::Plugin::PluginInterface;

namespace InMemoryScanner
{
    Config::Config(const PluginInterface* pluginInterface)
        : logger(pluginInterface->newNamedLogger(INMEMORY_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
    }

    void Config::parseConfiguration(const IPluginConfig& config)
    {
        auto& rootNode = config.rootNode();
        signatureFile = rootNode["signature_file"].as<std::string>();
        outputPath = rootNode["output_path"].as<std::string>();
        dumpMemory = rootNode["dump_memory"].as<bool>(false);
        scanAllRegions = rootNode["scan_all_regions"].as<bool>(false);

        auto ignoredProcessesVec =
            rootNode["ignored_processes"].as<std::vector<std::string>>(std::vector<std::string>());
        std::copy(ignoredProcessesVec.begin(),
                  ignoredProcessesVec.end(),
                  std::inserter(ignoredProcesses, ignoredProcesses.end()));
        for (auto& element : ignoredProcesses)
        {
            logger->info("Got ignored process", {{"Name", element}});
        }
    }

    std::filesystem::path Config::getSignatureFile() const
    {
        return signatureFile;
    }

    std::filesystem::path Config::getOutputPath() const
    {
        return outputPath;
    }

    bool Config::isProcessIgnored(const std::string& processName) const
    {
        return ignoredProcesses.find(processName) != ignoredProcesses.end();
    }

    bool Config::isScanAllRegionsActivated() const
    {
        return scanAllRegions;
    }

    bool Config::isDumpingMemoryActivated() const
    {
        return dumpMemory;
    }

    void Config::overrideDumpMemoryFlag(bool value)
    {
        dumpMemory = value;
    }
}
