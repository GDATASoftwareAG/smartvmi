#include "VadTreeWin10.h"
#include <unordered_set>

VadTreeWin10::VadTreeWin10(std::shared_ptr<IKernelAccess> kernelAccess,
                           uint64_t eprocessBase,
                           pid_t pid,
                           std::string processName,
                           const std::shared_ptr<ILogging>& loggingLib)
    : kernelAccess(std::move(kernelAccess)),
      eprocessBase(eprocessBase),
      pid(pid),
      processName(std::move(processName)),
      logger(NEW_LOGGER(loggingLib))
{
}

std::unique_ptr<std::list<Vadt>> VadTreeWin10::getAllVadts()
{
    auto vadtList = std::make_unique<std::list<Vadt>>();
    std::list<uint64_t> nextVadEntries;
    std::unordered_set<uint64_t> visitedVadVAs;
    auto nodeAddress = kernelAccess->extractVadTreeRootAddress(eprocessBase);
    nextVadEntries.push_back(nodeAddress);
    while (!nextVadEntries.empty())
    {
        auto currentVadEntryBaseVA = nextVadEntries.back();
        nextVadEntries.pop_back();

        auto insertionResult = visitedVadVAs.insert(currentVadEntryBaseVA);
        if (!insertionResult.second)
        {
            logger->warning("Cycle detected! Vad entry already visited",
                            {logfield::create("VadEntryBaseVA", Convenience::intToHex(currentVadEntryBaseVA))});
            continue;
        }

        addr_t leftChildAddress = 0;
        addr_t rightChildAddress = 0;
        try
        {
            std::tie(leftChildAddress, rightChildAddress) =
                kernelAccess->extractMmVadShortChildNodeAddresses(currentVadEntryBaseVA);
        }
        catch (const std::exception& e)
        {
            logger->warning("Unable to extract process",
                            {logfield::create("ProcessName", processName),
                             logfield::create("ProcessId", static_cast<int64_t>(pid)),
                             logfield::create("_MMVAD_SHORT", Convenience::intToHex(currentVadEntryBaseVA)),
                             logfield::create("exception", e.what())});
            continue;
        }
        if (leftChildAddress != 0)
        {
            nextVadEntries.push_back(leftChildAddress);
        }
        if (rightChildAddress != 0)
        {
            nextVadEntries.push_back(rightChildAddress);
        }

        try
        {
            auto currentVad = createVadt(currentVadEntryBaseVA);
            vadtList->push_back(std::move(*currentVad));
        }
        catch (const std::exception& e)
        {
            logger->warning("Unable to create Vadt object for process",
                            {logfield::create("ProcessName", processName),
                             logfield::create("ProcessId", static_cast<int64_t>(pid)),
                             logfield::create("exception", e.what())});
        }
    }
    return vadtList;
}

bool vadEntryIsFileBacked(bool imageFlag, bool fileFlag)
{
    return imageFlag || fileFlag;
}

std::unique_ptr<Vadt> VadTreeWin10::createVadt(uint64_t vadEntryBaseVA) const
{
    auto vadt = std::make_unique<Vadt>();
    auto vadShortBaseVA = kernelAccess->getVadShortBaseVA(vadEntryBaseVA);
    std::tie(vadt->startingVPN, vadt->endingVPN) = kernelAccess->extractMmVadShortVpns(vadShortBaseVA);
    vadt->protection = static_cast<ProtectionValues>(kernelAccess->extractProtectionFlagValue(vadShortBaseVA));
    vadt->isFileBacked = false;
    vadt->isBeingDeleted = false;
    vadt->isSharedMemory = !kernelAccess->extractIsPrivateMemory(vadShortBaseVA);
    vadt->isProcessBaseImage = false;

    vadt->vadEntryBaseVA = vadEntryBaseVA;

    if (vadt->isSharedMemory)
    {
        auto controlAreaBaseVA = kernelAccess->extractControlAreaBasePointer(vadEntryBaseVA);
        auto imageFlag = kernelAccess->extractIsImage(controlAreaBaseVA);
        auto fileFlag = kernelAccess->extractIsFile(controlAreaBaseVA);
        if (vadEntryIsFileBacked(imageFlag, fileFlag))
        {
            logger->debug("Is file backed",
                          {
                              logfield::create("mmSectionFlags.Image", Convenience::intToHex(imageFlag)),
                              logfield::create("mmSectionFlags.File", Convenience::intToHex(fileFlag)),
                          });
            vadt->isFileBacked = true;
            try
            {
                auto filePointerObjectAddress = kernelAccess->extractFilePointerObjectAddress(controlAreaBaseVA);
                vadt->fileName = kernelAccess->extractFileName(filePointerObjectAddress);

                auto imageFilePointerFromEprocess = kernelAccess->extractImageFilePointer(eprocessBase);
                auto imageFilePointerFromVad = filePointerObjectAddress;
                vadt->isProcessBaseImage = imageFilePointerFromEprocess == imageFilePointerFromVad;
            }
            catch (const std::exception& e)
            {
                logger->warning("Unable to extract file name for VAD",
                                {
                                    logfield::create("ProcessName", processName),
                                    logfield::create("ProcessId", static_cast<int64_t>(pid)),
                                    logfield::create("vadEntryBaseVA", vadEntryBaseVA),
                                    logfield::create("exception", e.what()),
                                });
            }
        }
        vadt->isBeingDeleted = kernelAccess->extractIsBeingDeleted(controlAreaBaseVA);
    }
    return vadt;
}
