#ifndef VMICORE_VADTREEWIN10_H
#define VMICORE_VADTREEWIN10_H

#include "../../io/ILogging.h"
#include "KernelAccess.h"
#include "Vadt.h"
#include <list>
#include <memory>

class VadTreeWin10
{
  public:
    VadTreeWin10(std::shared_ptr<IKernelAccess> kernelAccess,
                 uint64_t eprocessBase,
                 pid_t pid,
                 std::string processName,
                 const std::shared_ptr<ILogging>& loggingLib);

    std::unique_ptr<std::list<Vadt>> getAllVadts();

  private:
    std::shared_ptr<IKernelAccess> kernelAccess;
    uint64_t eprocessBase;
    pid_t pid;
    std::string processName;
    std::unique_ptr<ILogger> logger;

    [[nodiscard]] std::unique_ptr<Vadt> createVadt(uint64_t vadEntryBaseVA) const;
};

#endif // VMICORE_VADTREEWIN10_H
