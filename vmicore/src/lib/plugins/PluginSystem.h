#ifndef VMICORE_PLUGINSYSTEM_H
#define VMICORE_PLUGINSYSTEM_H

#include "../config/IConfigParser.h"
#include "../io/IEventStream.h"
#include "../io/ILogging.h"
#include "../io/file/LegacyLogging.h"
#include "../os/IActiveProcessesSupervisor.h"
#include "../vmi/InterruptEventSupervisor.h"
#include "../vmi/LibvmiInterface.h"
#include "PluginException.h"
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <vmicore/plugins/IPlugin.h>
#include <vmicore/plugins/PluginInterface.h>

namespace VmiCore
{
    class IPluginSystem : public Plugin::PluginInterface
    {
      public:
        ~IPluginSystem() override = default;

        virtual void
        initializePlugins(const std::map<std::string, std::vector<std::string>, std::less<>>& pluginArgs) = 0;

        virtual void passProcessStartEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) = 0;

        virtual void passProcessTerminationEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) = 0;

        virtual void unloadPlugins() = 0;

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

        void initializePlugins(const std::map<std::string, std::vector<std::string>, std::less<>>& pluginArgs) override;

        void passProcessStartEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) override;

        void passProcessTerminationEventToRegisteredPlugins(
            std::shared_ptr<const ActiveProcessInformation> processInformation) override;

        void unloadPlugins() override;

      private:
        std::shared_ptr<IConfigParser> configInterface;
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IInterruptEventSupervisor> interruptEventSupervisor;
        std::shared_ptr<IFileTransport> fileTransport;
        std::vector<std::function<void(std::shared_ptr<const ActiveProcessInformation>)>>
            registeredProcessStartCallbacks;
        std::vector<std::function<void(std::shared_ptr<const ActiveProcessInformation>)>>
            registeredProcessTerminationCallbacks;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;
        std::vector<std::pair<std::string, std::unique_ptr<Plugin::IPlugin>>> plugins;

        [[nodiscard]] std::unique_ptr<std::string> getResultsDir() const override;

        [[nodiscard]] std::unique_ptr<std::vector<uint8_t>>
        readPagesWithUnmappedRegionPadding(uint64_t pageAlignedVA, uint64_t cr3, uint64_t numberOfPages) const;

        [[nodiscard]] std::unique_ptr<std::vector<uint8_t>>
        readProcessMemoryRegion(pid_t pid, addr_t address, size_t numberOfBytes) const override;

        [[nodiscard]] std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
        getRunningProcesses() const override;

        void registerProcessStartEvent(
            const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& startCallback) override;

        void registerProcessTerminationEvent(
            const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& terminationCallback) override;

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

        void initializePlugin(const std::string& pluginName,
                              std::shared_ptr<Plugin::IPluginConfig> config,
                              const std::vector<std::string>& args);
    };
}

#endif // VMICORE_PLUGINSYSTEM_H
