#ifndef VMICORE_PLUGINSYSTEM_H
#define VMICORE_PLUGINSYSTEM_H

#include "../config/IConfigParser.h"
#include "../io/IEventStream.h"
#include "../io/ILogging.h"
#include "../io/file/LegacyLogging.h"
#include "../os/IActiveProcessesSupervisor.h"
#include "../vmi/InterruptEventSupervisor.h"
#include "../vmi/LibvmiInterface.h"
#include <vector>
#include <vmicore/plugins/PluginInterface.h>

namespace VmiCore
{
    class IPluginSystem : public Plugin::PluginInterface
    {
      public:
        ~IPluginSystem() override = default;

        virtual void initializePlugin(const std::string& pluginName,
                                      std::shared_ptr<Plugin::IPluginConfig> config,
                                      const std::vector<std::string>& args) = 0;

        virtual void passProcessStartEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) = 0;

        virtual void passProcessTerminationEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) = 0;

        virtual void passShutdownEventToRegisteredPlugins() = 0;

      protected:
        IPluginSystem() = default;
    };

    class PluginSystem : public IPluginSystem
    {
      public:
        PluginSystem(std::shared_ptr<IConfigParser> configInterface,
                     std::shared_ptr<ILibvmiInterface> vmiInterface,
                     std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                     std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor,
                     std::shared_ptr<IFileTransport> pluginLogging,
                     std::shared_ptr<ILogging> loggingLib,
                     std::shared_ptr<IEventStream> eventStream);

        ~PluginSystem() override;

        void initializePlugin(const std::string& pluginName,
                              std::shared_ptr<Plugin::IPluginConfig> config,
                              const std::vector<std::string>& args) override;

        void passProcessStartEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) override;

        void passProcessTerminationEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) override;

        void passShutdownEventToRegisteredPlugins() override;

      private:
        std::shared_ptr<IConfigParser> configInterface;
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor;
        std::shared_ptr<IFileTransport> fileTransport;
        std::vector<Plugin::processStartCallback_f> registeredProcessStartCallbacks;
        std::vector<Plugin::processTerminationCallback_f> registeredProcessTerminationCallbacks;
        std::vector<Plugin::shutdownCallback_f> registeredShutdownCallbacks;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;

        [[nodiscard]] std::unique_ptr<std::string> getResultsDir() const override;

        [[nodiscard]] std::unique_ptr<std::vector<uint8_t>>
        readPagesWithUnmappedRegionPadding(uint64_t pageAlignedVA, uint64_t cr3, uint64_t numberOfPages) const;

        [[nodiscard]] std::unique_ptr<std::vector<uint8_t>>
        readProcessMemoryRegion(pid_t pid, addr_t address, size_t numberOfBytes) const override;

        [[nodiscard]] std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
        getRunningProcesses() const override;

        void registerProcessStartEvent(Plugin::processStartCallback_f startCallback) override;

        void registerProcessTerminationEvent(Plugin::processTerminationCallback_f terminationCallback) override;

        void registerShutdownEvent(Plugin::shutdownCallback_f shutdownCallback) override;

        std::shared_ptr<IBreakpoint>
        createBreakpoint(uint64_t targetVA,
                         uint64_t processDtb,
                         const std::function<BpResponse(IInterruptEvent&)>& callbackFunction) override;

        [[nodiscard]] std::unique_ptr<ILogger> newNamedLogger(std::string_view name) const override;

        void writeToFile(const std::string& filename, const std::string& message) const override;

        void writeToFile(const std::string& filename, const std::vector<uint8_t>& data) const override;

        void sendErrorEvent(std::string_view message) const override;

        void sendInMemDetectionEvent(std::string_view message) const override;

        [[nodiscard]] std::shared_ptr<IIntrospectionAPI> getIntrospectionAPI() const override;
    };
}

#endif // VMICORE_PLUGINSYSTEM_H
