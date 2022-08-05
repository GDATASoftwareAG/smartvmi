#ifndef VMICORE_IACTIVEPROCESSESSUPERVISOR_H
#define VMICORE_IACTIVEPROCESSESSUPERVISOR_H

#include "ActiveProcessInformation.h"
#include "MemoryRegion.h"
#include <cstdint>
#include <memory>
#include <vector>

class IActiveProcessesSupervisor
{
  public:
    virtual ~IActiveProcessesSupervisor() = default;

    virtual void initialize() = 0;

    virtual std::shared_ptr<ActiveProcessInformation> getProcessInformationByPid(pid_t pid) = 0;

    virtual std::shared_ptr<ActiveProcessInformation> getProcessInformationByBase(uint64_t base) = 0;

    virtual void addNewProcess(uint64_t base) = 0;

    virtual void removeActiveProcess(uint64_t base) = 0;

    virtual std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> getActiveProcesses() = 0;

  protected:
    IActiveProcessesSupervisor() = default;
};

#endif // VMICORE_IACTIVEPROCESSESSUPERVISOR_H
