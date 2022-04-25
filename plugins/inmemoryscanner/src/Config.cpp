#include "Config.h"
#include "Filenames.h"
#include <algorithm>

Config::Config(const PluginInterface* pluginInterface) : pluginInterface(pluginInterface) {}

void Config::parseConfiguration(const IPluginConfig& config)
{
    signatureFile = config.getString("signature_file").value();
    outputPath = config.getString("output_path").value();
    dumpMemory = toBool(config.getString("dump_memory").value_or("false"));
    scanAllRegions = toBool(config.getString("scan_all_regions").value_or("false"));
    try
    {
        maximumScanSize = std::stoul(config.getString("maximum_scan_size").value_or("52428800")); // 50MB
    }
    catch (const std::invalid_argument&)
    {
        throw ConfigException("Configuration maximum_scan_size has invalid type");
    }
    catch (const std::out_of_range&)
    {
        throw ConfigException("Configuration maximum_scan_size is too big");
    }
    auto ignoredProcessesVec = config.getStringSequence("ignored_processes").value_or(std::vector<std::string>());
    std::copy(ignoredProcessesVec.begin(),
              ignoredProcessesVec.end(),
              std::inserter(ignoredProcesses, ignoredProcesses.end()));
    for (auto& element : ignoredProcesses)
    {
        pluginInterface->logMessage(
            LogLevel::info, LOG_FILENAME, std::string("Got ignored process \"").append(element).append("\""));
    }
}

bool Config::toBool(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str == "true")
    {
        return true;
    }
    if (str == "false")
    {
        return false;
    }
    if (str == "1")
    {
        return true;
    }
    if (str == "0")
    {
        return false;
    }

    throw ConfigException("String \"" + str + "\" cannot be converted to bool");
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

uint64_t Config::getMaximumScanSize() const
{
    return maximumScanSize;
}
