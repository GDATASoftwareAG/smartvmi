#pragma once

#include <PluginInterface.h>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>

class ConfigException : public std::runtime_error
{
  public:
    explicit ConfigException(const std::string& Message) : std::runtime_error(Message.c_str()){};
};

class IConfig
{
  public:
    virtual ~IConfig() = default;

    virtual void parseConfiguration(const Plugin::IPluginConfig& config) = 0;

    [[nodiscard]] virtual std::map<std::string, std::vector<std::string>> getHookTargets(const std::string& processName) const = 0;

    [[nodiscard]] virtual std::filesystem::path getOutputPath() const = 0;

    [[nodiscard]] virtual std::string getInitialProcessName() const = 0;

  protected:
    IConfig() = default;
};

class Config : public IConfig
{
  public:
    explicit Config(const Plugin::PluginInterface* pluginInterface);

    ~Config() override = default;

    void parseConfiguration(const Plugin::IPluginConfig& config) override;

    [[nodiscard]] std::map<std::string, std::vector<std::string>> getHookTargets(const std::string& processName) const override;

    [[nodiscard]] std::filesystem::path getOutputPath() const override;

    [[nodiscard]] std::string getInitialProcessName() const override;

  private:
    const Plugin::PluginInterface* pluginInterface;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> hookList;
    std::filesystem::path outputPath;
    std::string initialProcessName;
};
