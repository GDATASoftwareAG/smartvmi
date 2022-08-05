#ifndef VMICORE_WINDOWS_ACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_WINDOWS_ACTIVEPROCESSESSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogging.h"
#include "../../io/grpc/GRPCLogger.h"
#include "../../vmi/LibvmiInterface.h"
#include "../IActiveProcessesSupervisor.h"
#include "Constants.h"
#include "VadTreeWin10.h"
#include <map>
#include <memory>

namespace Windows
{
    class ActiveProcessesSupervisor : public IActiveProcessesSupervisor
    {
      public:
        ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                  std::shared_ptr<IKernelAccess> kernelAccess,
                                  std::shared_ptr<ILogging> loggingLib,
                                  std::shared_ptr<IEventStream> eventStream);

        void initialize() override;

        std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) override;

        std::shared_ptr<ActiveProcessInformation> getProcessInformationByBase(uint64_t eprocessBase) override;

        void addNewProcess(uint64_t eprocessBase) override;

        void removeActiveProcess(uint64_t eprocessBase) override;

        std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> getActiveProcesses() override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<IKernelAccess> kernelAccess;
        std::map<pid_t, std::shared_ptr<ActiveProcessInformation>> processInformationByPid;
        std::map<uint64_t, pid_t> pidsByEprocessBase;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<ILogging> loggingLib;
        std::shared_ptr<IEventStream> eventStream;

        [[nodiscard]] bool isProcessActive(uint64_t eprocessBase) const;

        [[nodiscard]] std::unique_ptr<ActiveProcessInformation> extractProcessInformation(uint64_t eprocessBase) const;

        [[nodiscard]] std::unique_ptr<std::string> extractProcessPath(uint64_t eprocessBase) const;

        [[nodiscard]] static std::unique_ptr<std::string> splitProcessFileNameFromPath(const std::string& path);
    };
}

#endif // VMICORE_WINDOWS_ACTIVEPROCESSESSUPERVISOR_H
