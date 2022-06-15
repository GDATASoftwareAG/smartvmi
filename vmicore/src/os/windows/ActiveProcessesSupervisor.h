#ifndef VMICORE_ACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_ACTIVEPROCESSESSUPERVISOR_H

#include "../../io/IEventStream.h"
#include "../../io/ILogging.h"
#include "../../io/grpc/GRPCLogger.h"
#include "../../vmi/LibvmiInterface.h"
#include "VadTreeWin10.h"
#include <map>
#include <memory>

struct ActiveProcessInformation
{
    uint64_t eprocessBase;
    uint64_t processCR3;
    pid_t pid;
    pid_t parentPid;
    std::string name;
    std::unique_ptr<std::string> fullName;
    VmiUnicodeStruct processPath;
    std::unique_ptr<VadTreeWin10> vadTree;
};

class IActiveProcessesSupervisor
{
  public:
    virtual ~IActiveProcessesSupervisor() = default;

    virtual void initialize() = 0;

    virtual std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) = 0;

    virtual std::shared_ptr<ActiveProcessInformation> getProcessInformationByEprocessBase(uint64_t eprocessBase) = 0;

    virtual void addNewProcess(uint64_t eprocessBase) = 0;

    virtual void removeActiveProcess(uint64_t eprocessBase) = 0;

    virtual std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> getActiveProcesses() = 0;

  protected:
    IActiveProcessesSupervisor() = default;
};

class ActiveProcessesSupervisor : public IActiveProcessesSupervisor
{
  public:
    ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                              std::shared_ptr<IKernelAccess> kernelAccess,
                              std::shared_ptr<ILogging> loggingLib,
                              std::shared_ptr<IEventStream> eventStream);

    void initialize() override;

    std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) override;

    std::shared_ptr<ActiveProcessInformation> getProcessInformationByEprocessBase(uint64_t eprocessBase) override;

    void addNewProcess(uint64_t eprocessBase) override;

    void removeActiveProcess(uint64_t eprocessBase) override;

    std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> getActiveProcesses() override;

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

    [[nodiscard]] VmiUnicodeStruct extractProcessPath(uint64_t eprocessBase) const;

    [[nodiscard]] static std::unique_ptr<std::string> splitProcessFileNameFromPath(const std::string_view& path);
};

#endif // VMICORE_ACTIVEPROCESSESSUPERVISOR_H
