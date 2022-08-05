#ifndef VMICORE_PLUGININTERFACE_H
#define VMICORE_PLUGININTERFACE_H

#include "../os/ActiveProcessInformation.h"
#include "IPluginConfig.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

constexpr uint8_t VMI_PLUGIN_API_VERSION = 12;

namespace Plugin
{
    using virtual_address_t = uint64_t;

    using processTerminationCallback_f = void (*)(pid_t processPid, const char* processName);

    using cr3ChangeCallback_f = void (*)(uint64_t newCR3);

    using shutdownCallback_f = void (*)();

    struct PluginDetails
    {
        unsigned int apiVersion;
        const char* pluginName;
        const char* pluginVersion;
    } __attribute__((aligned(128)));

    enum class LogLevel
    {
        debug,
        info,
        warning,
        error
    };

    class PluginInterface
    {
      public:
        virtual ~PluginInterface() = default;

        [[nodiscard]] virtual std::unique_ptr<std::vector<uint8_t>>
        readProcessMemoryRegion(pid_t pid, virtual_address_t address, size_t numberOfBytes) const = 0;

        [[nodiscard]] virtual std::unique_ptr<std::vector<MemoryRegion>> getProcessMemoryRegions(pid_t pid) const = 0;

        [[nodiscard]] virtual std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
        getRunningProcesses() const = 0;

        virtual void registerProcessTerminationEvent(processTerminationCallback_f terminationCallback) = 0;

        virtual void registerShutdownEvent(shutdownCallback_f shutdownCallback) = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string> getResultsDir() const = 0;

        virtual void writeToFile(const std::string& filename, const std::string& message) const = 0;

        virtual void writeToFile(const std::string& filename, const std::vector<uint8_t>& data) const = 0;

        virtual void logMessage(LogLevel logLevel, const std::string& filename, const std::string& message) const = 0;

        virtual void sendErrorEvent(const std::string& message) const = 0;

        virtual void sendInMemDetectionEvent(const std::string& message) const = 0;

      protected:
        PluginInterface() = default;
    };
}

#endif // VMICORE_PLUGININTERFACE_H
