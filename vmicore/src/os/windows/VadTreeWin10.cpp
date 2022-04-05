#include "VadTreeWin10.h"
#include "../../offsets/OffsetDefinitions.h"
#include <unordered_set>

VadTreeWin10::VadTreeWin10(std::shared_ptr<IKernelObjectExtractorWin10> kernelObjectExtractor,
                           uint64_t eprocessBase,
                           pid_t pid,
                           std::string processName,
                           std::shared_ptr<ILogging> loggingLib)
    : kernelObjectExtractor(std::move(kernelObjectExtractor)),
      pid(pid),
      processName(std::move(processName)),
      logger(NEW_LOGGER(loggingLib))
{
    vadTreeRootNodeAddressLocation = eprocessBase + OffsetDefinitionsWin10::_EPROCESS::VadRoot;
    imageFilePointerAddressLocation = eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ImageFilePointer;
}

std::unique_ptr<std::list<Vadt>> VadTreeWin10::getAllVadts()
{
    auto vadtList = std::make_unique<std::list<Vadt>>();
    std::list<uint64_t> nextVadEntries;
    std::unordered_set<uint64_t> visitedVadVAs;
    auto nodeAddress = kernelObjectExtractor->getVadTreeRootAddress(vadTreeRootNodeAddressLocation);
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

        std::unique_ptr<_MMVAD_SHORT> vadShortEntry;
        try
        {
            vadShortEntry = kernelObjectExtractor->extractMmVadShort(currentVadEntryBaseVA);
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
        auto leftChildAddress = reinterpret_cast<uint64_t>(vadShortEntry->VadNode.balancedNodeChildren.Left);
        if (leftChildAddress != 0)
        {
            nextVadEntries.push_back(leftChildAddress);
        }
        auto rightChildAddress = reinterpret_cast<uint64_t>(vadShortEntry->VadNode.balancedNodeChildren.Right);
        if (rightChildAddress != 0)
        {
            nextVadEntries.push_back(rightChildAddress);
        }

        try
        {
            auto currentVad = createVadt(*vadShortEntry, currentVadEntryBaseVA);
            vadtList->push_back(*currentVad);
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

uint64_t getStartingVpn(const _MMVAD_SHORT& vadShortEntry)
{
    return (static_cast<uint64_t>(vadShortEntry.StartingVpnHigh) << sizeof(vadShortEntry.StartingVpn) * 8) +
           vadShortEntry.StartingVpn;
}

uint64_t getEndingVpn(const _MMVAD_SHORT& vadShortEntry)
{
    return (static_cast<uint64_t>(vadShortEntry.EndingVpnHigh) << sizeof(vadShortEntry.EndingVpn) * 8) +
           vadShortEntry.EndingVpn;
}

bool vadEntryIsFileBacked(const _CONTROL_AREA& controlArea)
{
    return controlArea.u.Flags.Image == 1 || controlArea.u.Flags.File == 1;
}

bool vadEntryIsBeingDeleted(const _CONTROL_AREA& controlArea)
{
    return controlArea.u.Flags.BeingDeleted == 1;
}

std::unique_ptr<Vadt> VadTreeWin10::createVadt(const _MMVAD_SHORT& vadShortEntry, uint64_t vadEntryBaseVA) const
{
    auto vadt = std::make_unique<Vadt>();
    vadt->startingVPN = getStartingVpn(vadShortEntry);
    vadt->endingVPN = getEndingVpn(vadShortEntry);
    vadt->protection = ProtectionValues(vadShortEntry.u.VadFlags.Protection);
    vadt->isFileBacked = false;
    vadt->isBeingDeleted = false;
    vadt->isSharedMemory = !vadShortEntry.u.VadFlags.PrivateMemory;
    vadt->isProcessBaseImage = false;

    vadt->vadEntryBaseVA = vadEntryBaseVA;

    if (vadt->isSharedMemory)
    {
        auto controlArea = extractControlAreaFromVadEntry(vadEntryBaseVA);
        if (vadEntryIsFileBacked(*controlArea))
        {
            logger->debug(
                "Is file backed",
                {
                    logfield::create("controlArea.u.Flags.Image", Convenience::intToHex(controlArea->u.Flags.Image)),
                    logfield::create("controlArea.u.Flags.File", Convenience::intToHex(controlArea->u.Flags.File)),
                });
            vadt->isFileBacked = true;
            try
            {
                auto fileObject = extractFileObject(*controlArea);
                vadt->fileName = *extractVadFileName(*fileObject);

                auto imageFilePointerFromEprocess =
                    kernelObjectExtractor->getImageFilePointer(imageFilePointerAddressLocation);
                auto imageFilePointerFromVad =
                    removeReferenceCountFromExFastRef(reinterpret_cast<uint64_t>(controlArea->FilePointer.Object));
                vadt->isProcessBaseImage = imageFilePointerFromEprocess == imageFilePointerFromVad;
            }
            catch (const std::exception& e)
            {
                vadt->fileName = "unknownFilename";
                logger->warning("Unable to extract file name for VAD",
                                {
                                    logfield::create("ProcessName", processName),
                                    logfield::create("ProcessId", static_cast<int64_t>(pid)),
                                    logfield::create("vadEntryBaseVA", vadEntryBaseVA),
                                    logfield::create("exception", e.what()),
                                });
            }
        }
        vadt->isBeingDeleted = vadEntryIsBeingDeleted(*controlArea);
    }
    return vadt;
}

std::unique_ptr<_CONTROL_AREA> VadTreeWin10::extractControlAreaFromVadEntry(uint64_t vadEntryBaseVA) const
{
    logger->debug("getMmVad from vadEntryBaseVA",
                  {logfield::create("vadEntryBaseVA", Convenience::intToHex(vadEntryBaseVA))});
    std::unique_ptr<_MMVAD> vadEntry;
    try
    {
        vadEntry = kernelObjectExtractor->extractMmVad(vadEntryBaseVA);
    }
    catch (const VmiException&)
    {
        throw VmiException("Unable to extract _MMVAD @ " + Convenience::intToHex(vadEntryBaseVA));
    }
    logger->debug("getMmVadSubsection from vadEntry",
                  {logfield::create("MmVadSubsection", Convenience::intToHex(vadEntry->Subsection))});
    std::unique_ptr<_SUBSECTION> subsection;
    try
    {
        subsection = kernelObjectExtractor->extractMmVadSubsection(reinterpret_cast<uint64_t>(vadEntry->Subsection));
    }
    catch (const VmiException&)
    {
        throw VmiException("Unable to extract _SUBSECTION @ " + Convenience::intToHex(vadEntry->Subsection));
    }
    logger->debug("controlArea from vadEntry",
                  {logfield::create("controlArea", Convenience::intToHex(subsection->ControlArea))});
    std::unique_ptr<_CONTROL_AREA> controlArea;
    try
    {
        controlArea = kernelObjectExtractor->extractControlArea(reinterpret_cast<uint64_t>(subsection->ControlArea));
    }
    catch (const VmiException&)
    {
        throw VmiException("Unable to extract _CONTROL_AREA @ " + Convenience::intToHex(subsection->ControlArea));
    }
    return controlArea;
}

uint64_t VadTreeWin10::removeReferenceCountFromExFastRef(uint64_t exFastRefValue)
{
    return exFastRefValue & ~(static_cast<uint64_t>(0xF));
}

std::unique_ptr<std::string> VadTreeWin10::extractVadFileName(const _FILE_OBJECT& fileObject) const
{
    logger->debug("extractVadFileName",
                  {
                      logfield::create("fileObject.FileName.Buffer", Convenience::intToHex(fileObject.FileName.Buffer)),
                      logfield::create("fileObject.FileName.Length", static_cast<uint64_t>(fileObject.FileName.Length)),

                  });
    auto fileName = std::make_unique<std::string>("File name unavailable");
    try
    {
        fileName = kernelObjectExtractor->extractWString(reinterpret_cast<uint64_t>(fileObject.FileName.Buffer),
                                                         fileObject.FileName.Length);
        logger->debug("Vad FileName", {logfield::create("FileName", *fileName)});
    }
    catch (const VmiException& e)
    {
        logger->warning("Unable to extract file name", {logfield::create("exception", e.what())});
    }
    return fileName;
}

std::unique_ptr<_FILE_OBJECT> VadTreeWin10::extractFileObject(const _CONTROL_AREA& controlArea) const
{
    std::unique_ptr<_FILE_OBJECT> fileObject;
    try
    {
        fileObject = kernelObjectExtractor->extractFileObject(
            removeReferenceCountFromExFastRef(reinterpret_cast<uint64_t>(controlArea.FilePointer.Object)));
    }
    catch (const VmiException&)
    {
        throw VmiException("Unable to extract _FILE_OBJECT @ " + Convenience::intToHex(controlArea.FilePointer.Object));
    }
    return fileObject;
}
