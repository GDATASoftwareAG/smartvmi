#ifndef VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogger.h"
#include "../../io/ILogging.h"
#include "../../vmi/LibvmiInterface.h"
#include "../IActiveProcessesSupervisor.h"
#include "PathExtractor.h"
#include <map>
#include <memory>

namespace VmiCore::Linux
{
    class ActiveProcessesSupervisor : public IActiveProcessesSupervisor
    {
      public:
        ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                  std::shared_ptr<ILogging> loggingLib,
                                  std::shared_ptr<IEventStream> eventStream);

        void initialize() override;

        [[nodiscard]] std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) const override;

        [[nodiscard]] std::shared_ptr<ActiveProcessInformation>
        getProcessInformationByBase(uint64_t taskStruct) const override;

        void addNewProcess(uint64_t taskStruct) override;

        void removeActiveProcess(uint64_t taskStruct) override;

        [[nodiscard]] std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
        getActiveProcesses() const override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<ILogging> logging;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;
        PathExtractor pathExtractor;
        std::map<pid_t, std::shared_ptr<ActiveProcessInformation>> processInformationByPid;
        std::map<uint64_t, pid_t> pidsByTaskStruct;

        [[nodiscard]] std::unique_ptr<ActiveProcessInformation> extractProcessInformation(uint64_t taskStruct);

        [[nodiscard]] pid_t extractPid(uint64_t taskStruct) const;

        [[nodiscard]] std::string_view splitProcessFileNameFromPath(const std::string_view& path) const;
    };
}

#endif // VMICORE_LINUX_ACTIVEPROCESSESSUPERVISOR_H
