#include "VadTreeWin10.h"
#include "../../vmi/VmiException.h"
#include "../PageProtection.h"
#include "../PagingDefinitions.h"
#include <fmt/core.h>
#include <unordered_set>
#include <vmicore/filename.h>

namespace VmiCore::Windows
{
    VadTreeWin10::VadTreeWin10(std::shared_ptr<IKernelAccess> kernelAccess,
                               uint64_t eprocessBase,
                               pid_t pid,
                               std::string processName,
                               const std::shared_ptr<ILogging>& logging)
        : kernelAccess(std::move(kernelAccess)),
          eprocessBase(eprocessBase),
          pid(pid),
          processName(std::move(processName)),
          logger(logging->newNamedLogger(FILENAME_STEM)),
          mmProtectToValue(this->kernelAccess->extractMmProtectToValue())
    {
    }

    std::unique_ptr<std::list<MemoryRegion>> VadTreeWin10::extractAllMemoryRegions() const
    {
        auto regions = std::make_unique<std::list<MemoryRegion>>();
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
                                {logfield::create("VadEntryBaseVA", fmt::format("{:#x}", currentVadEntryBaseVA))});
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
                                 logfield::create("_MMVAD_SHORT", fmt::format("{:#x}", currentVadEntryBaseVA)),
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
                const auto currentVad = createVadt(currentVadEntryBaseVA);

                const auto startAddress = currentVad->startingVPN << PagingDefinitions::numberOfPageIndexBits;
                const auto endAddress = ((currentVad->endingVPN + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
                const auto size = endAddress - startAddress + 1;
                logger->debug("Vadt element",
                              {logfield::create("startingVPN", fmt::format("{:#x}", currentVad->startingVPN)),
                               logfield::create("endingVPN", fmt::format("{:#x}", currentVad->endingVPN)),
                               logfield::create("startAddress", fmt::format("{:#x}", startAddress)),
                               logfield::create("endAddress", fmt::format("{:#x}", endAddress)),
                               logfield::create("size", static_cast<uint64_t>(size))});
                regions->emplace_back(startAddress,
                                      size,
                                      std::string(currentVad->fileName),
                                      std::make_unique<PageProtection>(mmProtectToValue.at(currentVad->protection),
                                                                       OperatingSystem::WINDOWS),
                                      currentVad->isSharedMemory,
                                      currentVad->isBeingDeleted,
                                      currentVad->isProcessBaseImage);
            }
            catch (const std::exception& e)
            {
                logger->warning("Unable to create Vadt object for process",
                                {logfield::create("ProcessName", processName),
                                 logfield::create("ProcessId", static_cast<int64_t>(pid)),
                                 logfield::create("exception", e.what())});
            }
        }

        return regions;
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
        vadt->protection = kernelAccess->extractProtectionFlagValue(vadShortBaseVA);
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
                                  logfield::create("mmSectionFlags.Image", fmt::format("{:#x}", imageFlag)),
                                  logfield::create("mmSectionFlags.File", fmt::format("{:#x}", fileFlag)),
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

    VmiUnicodeStruct VadTreeWin10::extractFileName(addr_t filePointerObjectAddress) const
    {
        try
        {
            return kernelAccess->extractFileName(filePointerObjectAddress);
        }
        catch (const VmiException&)
        {
            throw VmiException(fmt::format("Unable to extract Filename @ {:#x}", filePointerObjectAddress));
        }
    }
}
