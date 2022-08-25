#ifndef VMICORE_IACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_IACTIVEPROCESSESSUPERVISOR_H

#include <cstdint>
#include <memory>
#include <vector>
#include <vmicore/os/ActiveProcessInformation.h>

class IActiveProcessesSupervisor
{
  public:
    virtual ~IActiveProcessesSupervisor() = default;

    virtual void initialize() = 0;

    [[nodiscard]] virtual std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) const = 0;

    [[nodiscard]] virtual std::shared_ptr<ActiveProcessInformation>
    getProcessInformationByBase(uint64_t base) const = 0;

    virtual void addNewProcess(uint64_t base) = 0;

    virtual void removeActiveProcess(uint64_t base) = 0;

    [[nodiscard]] virtual std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
    getActiveProcesses() const = 0;

  protected:
    IActiveProcessesSupervisor() = default;
};

#endif // VMICORE_IACTIVEPROCESSESSUPERVISOR_H
