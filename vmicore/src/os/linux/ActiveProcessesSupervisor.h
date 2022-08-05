#ifndef VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogging.h"
#include "../../io/grpc/GRPCLogger.h"
#include "../../vmi/LibvmiInterface.h"
#include "../IActiveProcessesSupervisor.h"
#include <map>
#include <memory>

namespace Linux
{
    inline static constexpr auto systemPid = 0;

    class ActiveProcessesSupervisor : public IActiveProcessesSupervisor
    {
        inline static constexpr auto PATH_MAX_LINUX = 4096;

      public:
        ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                  std::shared_ptr<ILogging> loggingLib,
                                  std::shared_ptr<IEventStream> eventStream);

        void initialize() override;

        std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) override;

        std::shared_ptr<ActiveProcessInformation> getProcessInformationByBase(uint64_t taskStruct) override;

        void addNewProcess(uint64_t taskStruct) override;

        void removeActiveProcess(uint64_t taskStruct) override;

        std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> getActiveProcesses() override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::map<pid_t, std::shared_ptr<ActiveProcessInformation>> processInformationByPid;
        std::map<uint64_t, pid_t> pidsByTaskStruct;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<ILogging> loggingLib;
        std::shared_ptr<IEventStream> eventStream;

        [[nodiscard]] std::unique_ptr<ActiveProcessInformation> extractProcessInformation(uint64_t taskStruct) const;

        void extractMemoryRegions(std::vector<MemoryRegion>& regions, uint64_t mm) const;

        [[nodiscard]] pid_t extractPid(uint64_t taskStruct) const;

        [[nodiscard]] std::unique_ptr<std::string> extractDPath(uint64_t path) const;

        void createPath(uint64_t dentry, uint64_t mnt, char* buf) const;

        [[nodiscard]] std::unique_ptr<std::string> splitProcessFileNameFromPath(const std::string& path) const;
    };
}

#endif // VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H
