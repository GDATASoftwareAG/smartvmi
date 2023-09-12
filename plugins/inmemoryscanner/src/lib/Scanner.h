#pragma once

#include "Config.h"
#include "Dumping.h"
#include "OutputXML.h"
#include "Semaphore.h"
#include "YaraInterface.h"
#include <condition_variable>
#include <memory>
#include <vmicore/plugins/PluginInterface.h>
#include <yara/limits.h> // NOLINT(modernize-deprecated-headers)

namespace InMemoryScanner
{
    class Scanner
    {
      public:
        Scanner(VmiCore::Plugin::PluginInterface* pluginInterface,
                std::shared_ptr<IConfig> configuration,
                std::unique_ptr<YaraInterface> yaraEngine,
                std::unique_ptr<IDumping> dumping);

        [[nodiscard]] static std::unique_ptr<std::string> getFilenameFromPath(const std::string& path);

        void scanProcess(std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation);

        void scanAllProcesses();

        void saveOutput();

      private:
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::shared_ptr<IConfig> configuration;
        std::unique_ptr<YaraInterface> yaraEngine;
        OutputXML outputXml{};
        std::unique_ptr<IDumping> dumping;
        std::unique_ptr<VmiCore::ILogger> logger;
        std::unique_ptr<VmiCore::ILogger> inMemResultsLogger;
        Semaphore<std::mutex, std::condition_variable> semaphore =
            Semaphore<std::mutex, std::condition_variable>(YR_MAX_THREADS);

        [[nodiscard]] bool shouldRegionBeScanned(const VmiCore::MemoryRegion& memoryRegionDescriptor);

        void scanMemoryRegion(pid_t pid,
                              const std::string& processName,
                              const VmiCore::MemoryRegion& memoryRegionDescriptor);

        void logInMemoryResultToTextFile(const std::string& processName,
                                         VmiCore::pid_t pid,
                                         VmiCore::addr_t baseAddress,
                                         const std::vector<Rule>& results);
    };
}
