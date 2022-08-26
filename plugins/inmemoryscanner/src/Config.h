#pragma once

#include <filesystem>
#include <memory>
#include <set>
#include <stdexcept>
#include <vmicore/plugins/PluginInterface.h>

class ConfigException : public std::runtime_error
{
  public:
    explicit ConfigException(const std::string& Message) : std::runtime_error(Message.c_str()){};
};

class IConfig
{
  public:
    virtual ~IConfig() = default;

    virtual void parseConfiguration(const VmiCore::Plugin::IPluginConfig& config) = 0;

    [[nodiscard]] virtual std::filesystem::path getSignatureFile() const = 0;

    [[nodiscard]] virtual std::filesystem::path getOutputPath() const = 0;

    [[nodiscard]] virtual bool isProcessIgnored(const std::string& processName) const = 0;

    [[nodiscard]] virtual bool isScanAllRegionsActivated() const = 0;

    [[nodiscard]] virtual bool isDumpingMemoryActivated() const = 0;

    [[nodiscard]] virtual uint64_t getMaximumScanSize() const = 0;

    virtual void overrideDumpMemoryFlag(bool value) = 0;

  protected:
    IConfig() = default;
};

class Config : public IConfig
{
  public:
    explicit Config(const VmiCore::Plugin::PluginInterface* pluginInterface);

    ~Config() override = default;

    void parseConfiguration(const VmiCore::Plugin::IPluginConfig& config) override;

    [[nodiscard]] std::filesystem::path getSignatureFile() const override;

    [[nodiscard]] std::filesystem::path getOutputPath() const override;

    [[nodiscard]] bool isProcessIgnored(const std::string& processName) const override;

    [[nodiscard]] bool isScanAllRegionsActivated() const override;

    [[nodiscard]] bool isDumpingMemoryActivated() const override;

    [[nodiscard]] uint64_t getMaximumScanSize() const override;

    void overrideDumpMemoryFlag(bool value) override;

  private:
    const VmiCore::Plugin::PluginInterface* pluginInterface;
    std::filesystem::path outputPath;
    std::filesystem::path signatureFile;
    std::set<std::string> ignoredProcesses;
    bool dumpMemory{};
    bool scanAllRegions{};
    uint64_t maximumScanSize{};

    static bool toBool(std::string str);
};
