#ifndef VMICORE_VADTREEWIN10_H
#define VMICORE_VADTREEWIN10_H

#include "../../io/ILogging.h"
#include "KernelObjectExtractorWin10.h"
#include "Vadt.h"
#include <list>
#include <memory>

class VadTreeWin10
{
  public:
    VadTreeWin10(std::shared_ptr<IKernelObjectExtractorWin10> kernelObjectExtractor,
                 uint64_t eprocessBase,
                 pid_t pid,
                 std::string processName,
                 std::shared_ptr<ILogging> loggingLib);

    std::unique_ptr<std::list<Vadt>> getAllVadts();

  private:
    std::shared_ptr<IKernelObjectExtractorWin10> kernelObjectExtractor;
    pid_t pid;
    std::string processName;
    uint64_t vadTreeRootNodeAddressLocation;
    uint64_t imageFilePointerAddressLocation;
    std::unique_ptr<ILogger> logger;

    std::unique_ptr<Vadt> createVadt(const _MMVAD_SHORT& vadShortEntry, uint64_t vadEntryBaseVA) const;

    std::unique_ptr<_CONTROL_AREA> extractControlAreaFromVadEntry(uint64_t vadEntryBaseVA) const;

    static uint64_t removeReferenceCountFromExFastRef(uint64_t exFastRefValue);

    std::unique_ptr<_FILE_OBJECT> extractFileObject(const _CONTROL_AREA& controlArea) const;

    std::unique_ptr<std::string> extractVadFileName(const _FILE_OBJECT& fileObject) const;
};

#endif // VMICORE_VADTREEWIN10_H
