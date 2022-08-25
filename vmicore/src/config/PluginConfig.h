#ifndef VMICORE_PLUGINCONFIG_H
#define VMICORE_PLUGINCONFIG_H

#include <string>
#include <vmicore/plugins/IPluginConfig.h>
#include <yaml-cpp/yaml.h>

class PluginConfig : public Plugin::IPluginConfig
{
  public:
    explicit PluginConfig(const YAML::Node& pluginNode);

    ~PluginConfig() override = default;

    std::optional<std::string> getString(const std::string& element) const override;

    void overrideString(const std::string& element, const std::string& value) override;

    std::optional<std::vector<std::string>> getStringSequence(const std::string& element) const override;

  private:
    YAML::Node pluginRootNode;
};

#endif // VMICORE_PLUGINCONFIG_H
