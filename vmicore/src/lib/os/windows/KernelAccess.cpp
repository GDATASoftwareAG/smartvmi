#include "KernelAccess.h"
#include "../../vmi/VmiException.h"
#include "../PagingDefinitions.h"
#include "Constants.h"
#include <fmt/core.h>

namespace
{
    constexpr uint64_t exFastRefBits = 0xF;
}

namespace VmiCore::Windows
{
    KernelAccess::KernelAccess(std::shared_ptr<ILibvmiInterface> vmiInterface) : vmiInterface(std::move(vmiInterface))
    {
    }

    void KernelAccess::initWindowsOffsets()
    {
        kernelOffsets = KernelOffsets::init(vmiInterface);
    }

    addr_t KernelAccess::extractVadTreeRootAddress(addr_t eprocessBase) const
    {
        auto vadRoot = vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.VadRoot,
                                              vmiInterface->convertPidToDtb(systemPid));
        return vadRoot;
    }

    addr_t KernelAccess::extractImageFilePointer(addr_t eprocessBase) const
    {
        auto imageFilePointer = vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.ImageFilePointer,
                                                       vmiInterface->convertPidToDtb(systemPid));
        return imageFilePointer;
    }

    VmiUnicodeStruct KernelAccess::extractFileName(addr_t fileObjectBaseAddress) const
    {
        expectSaneKernelAddress(fileObjectBaseAddress, static_cast<const char*>(__func__));
        return vmiInterface->extractUnicodeStringAtVA(fileObjectBaseAddress + kernelOffsets.fileObject.FileName,
                                                      vmiInterface->convertPidToDtb(systemPid));
    }

    addr_t KernelAccess::extractControlAreaBasePointer(addr_t vadEntryBaseVA) const
    {
        expectSaneKernelAddress(vadEntryBaseVA, static_cast<const char*>(__func__));
        auto subSectionBaseAddress = vmiInterface->read64VA(vadEntryBaseVA + kernelOffsets.mmVad.Subsection,
                                                            vmiInterface->convertPidToDtb(systemPid));
        auto controlAreaBaseAddress = vmiInterface->read64VA(
            subSectionBaseAddress + kernelOffsets.subSection.ControlArea, vmiInterface->convertPidToDtb(systemPid));
        return controlAreaBaseAddress;
    }

    void KernelAccess::expectSaneKernelAddress(addr_t address, const char* caller)
    {
        if (address < PagingDefinitions::kernelspaceLowerBoundary)
        {
            throw std::invalid_argument(fmt::format("{}: {:#x} is not a kernel space address", caller, address));
        }
    }

    addr_t KernelAccess::extractFilePointerObjectAddress(addr_t controlAreaBaseVA) const
    {
        expectSaneKernelAddress(controlAreaBaseVA, static_cast<const char*>(__func__));
        auto filePointerObjectExFastRef = vmiInterface->read64VA(
            controlAreaBaseVA + kernelOffsets.controlArea.FilePointer + kernelOffsets.exFastRef.Object,
            vmiInterface->convertPidToDtb(systemPid));
        auto filePointerObjectAddress = removeReferenceCountFromExFastRef(filePointerObjectExFastRef);
        return filePointerObjectAddress;
    }

    uint64_t KernelAccess::removeReferenceCountFromExFastRef(uint64_t exFastRefValue)
    {
        return exFastRefValue & ~(exFastRefBits);
    }

    std::tuple<addr_t, addr_t> KernelAccess::extractMmVadShortChildNodeAddresses(addr_t currentVadEntryBaseVA) const
    {
        expectSaneKernelAddress(currentVadEntryBaseVA, static_cast<const char*>(__func__));
        auto leftChildAddress = vmiInterface->read64VA(currentVadEntryBaseVA + getVadNodeLeftChildOffset(),
                                                       vmiInterface->convertPidToDtb(systemPid));
        auto rightChildAddress = vmiInterface->read64VA(currentVadEntryBaseVA + getVadNodeRightChildOffset(),
                                                        vmiInterface->convertPidToDtb(systemPid));
        return {leftChildAddress, rightChildAddress};
    }

    std::tuple<uint64_t, uint64_t> KernelAccess::extractMmVadShortVpns(addr_t currentVadShortBaseVA) const
    {
        expectSaneKernelAddress(currentVadShortBaseVA, static_cast<const char*>(__func__));
        auto startingVpnHigh = vmiInterface->read8VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.StartingVpnHigh,
                                                     vmiInterface->convertPidToDtb(systemPid));
        auto endingVpnHigh = vmiInterface->read8VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.EndingVpnHigh,
                                                   vmiInterface->convertPidToDtb(systemPid));
        auto startingVpn = vmiInterface->read32VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.StartingVpn,
                                                  vmiInterface->convertPidToDtb(systemPid));
        auto endingVpn = vmiInterface->read32VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.EndingVpn,
                                                vmiInterface->convertPidToDtb(systemPid));
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
        uint64_t vadShortEndingVpn = (static_cast<uint64_t>(endingVpnHigh) << sizeof(endingVpn) * 8) + endingVpn;
        uint64_t vadShortStartingVpn =
            (static_cast<uint64_t>(startingVpnHigh) << sizeof(startingVpn) * 8) + startingVpn;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

        return {vadShortStartingVpn, vadShortEndingVpn};
    }

    addr_t KernelAccess::getVadShortBaseVA(addr_t vadEntryBaseVA) const
    {
        return vadEntryBaseVA + kernelOffsets.mmVad.mmVadShortBaseAddress;
    }

    addr_t KernelAccess::getCurrentProcessEprocessBase(addr_t currentListEntry) const
    {
        return currentListEntry - kernelOffsets.eprocess.ActiveProcessLinks;
    }

    addr_t KernelAccess::extractDirectoryTableBase(addr_t eprocessBase) const
    {
        return vmiInterface->read64VA(eprocessBase + kernelOffsets.kprocess.DirectoryTableBase,
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    pid_t KernelAccess::extractParentID(addr_t eprocessBase) const
    {
        return static_cast<pid_t>(
            vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.InheritedFromUniqueProcessId,
                                   vmiInterface->convertPidToDtb(systemPid)));
    }

    std::string KernelAccess::extractImageFileName(addr_t eprocessBase) const
    {
        return *vmiInterface->extractStringAtVA(eprocessBase + kernelOffsets.eprocess.ImageFileName,
                                                vmiInterface->convertPidToDtb(systemPid));
    }

    pid_t KernelAccess::extractPID(addr_t eprocessBase) const
    {
        return static_cast<pid_t>(vmiInterface->read32VA(eprocessBase + kernelOffsets.eprocess.UniqueProcessId,
                                                         vmiInterface->convertPidToDtb(systemPid)));
    }

    uint32_t KernelAccess::extractExitStatus(addr_t eprocessBase) const
    {
        return vmiInterface->read32VA(eprocessBase + kernelOffsets.eprocess.ExitStatus,
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    addr_t KernelAccess::extractSectionAddress(addr_t eprocessBase) const
    {
        return vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.SectionObject,
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    addr_t KernelAccess::extractControlAreaAddress(addr_t sectionAddress) const
    {
        expectSaneKernelAddress(sectionAddress, static_cast<const char*>(__func__));
        return vmiInterface->read64VA(sectionAddress + kernelOffsets.section.controlArea,
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    addr_t KernelAccess::extractControlAreaFilePointer(addr_t controlAreaAddress) const
    {
        expectSaneKernelAddress(controlAreaAddress, static_cast<const char*>(__func__));
        return vmiInterface->read64VA(controlAreaAddress + kernelOffsets.controlArea.FilePointer,
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    VmiUnicodeStruct KernelAccess::extractProcessPath(addr_t filePointerAddress) const
    {
        expectSaneKernelAddress(filePointerAddress, static_cast<const char*>(__func__));
        return vmiInterface->extractUnicodeStringAtVA(filePointerAddress + kernelOffsets.fileObject.FileName,
                                                      vmiInterface->convertPidToDtb(systemPid));
    }

    addr_t KernelAccess::getMmVadShortFlagsAddr(addr_t vadShortBaseVA) const
    {
        return vadShortBaseVA + kernelOffsets.mmVadShort.Flags;
    }

    uint8_t KernelAccess::extractProtectionFlagValue(addr_t vadShortBaseVA) const
    {
        auto flagsSize = vmiInterface->getStructSizeFromJson(KernelStructOffsets::mmvad_flags::structName);
        // As of now, there are 32 Protectionvalues
        assert((kernelOffsets.mmvadFlags.protection.endBit - kernelOffsets.mmvadFlags.protection.startBit) < 6);
        return static_cast<uint8_t>(extractFlagValue(getMmVadShortFlagsAddr(vadShortBaseVA),
                                                     flagsSize,
                                                     kernelOffsets.mmvadFlags.protection.startBit,
                                                     kernelOffsets.mmvadFlags.protection.endBit));
    }

    bool KernelAccess::extractIsPrivateMemory(addr_t vadShortBaseVA) const
    {
        auto flagsSize = vmiInterface->getStructSizeFromJson(KernelStructOffsets::mmvad_flags::structName);
        assert((kernelOffsets.mmvadFlags.privateMemory.endBit - kernelOffsets.mmvadFlags.privateMemory.startBit) == 1);
        return static_cast<bool>(extractFlagValue(getMmVadShortFlagsAddr(vadShortBaseVA),
                                                  flagsSize,
                                                  kernelOffsets.mmvadFlags.privateMemory.startBit,
                                                  kernelOffsets.mmvadFlags.privateMemory.endBit));
    }

    addr_t KernelAccess::getMmSectionFlagsAddr(addr_t controlAreaBaseVA) const
    {
        return controlAreaBaseVA + kernelOffsets.controlArea._mmsection_flags;
    }

    bool KernelAccess::extractIsBeingDeleted(addr_t controlAreaBaseVA) const
    {
        auto flagsSize = vmiInterface->getStructSizeFromJson(KernelStructOffsets::mmsection_flags::structName);
        assert((kernelOffsets.mmsectionFlags.beingDeleted.endBit -
                kernelOffsets.mmsectionFlags.beingDeleted.startBit) == 1);
        return static_cast<bool>(extractFlagValue(getMmSectionFlagsAddr(controlAreaBaseVA),
                                                  flagsSize,
                                                  kernelOffsets.mmsectionFlags.beingDeleted.startBit,
                                                  kernelOffsets.mmsectionFlags.beingDeleted.endBit));
    }

    bool KernelAccess::extractIsImage(addr_t controlAreaBaseVA) const
    {
        auto flagsSize = vmiInterface->getStructSizeFromJson(KernelStructOffsets::mmsection_flags::structName);
        assert((kernelOffsets.mmsectionFlags.image.endBit - kernelOffsets.mmsectionFlags.image.startBit) == 1);
        return static_cast<bool>(extractFlagValue(getMmSectionFlagsAddr(controlAreaBaseVA),
                                                  flagsSize,
                                                  kernelOffsets.mmsectionFlags.image.startBit,
                                                  kernelOffsets.mmsectionFlags.image.endBit));
    }

    bool KernelAccess::extractIsFile(addr_t controlAreaBaseVA) const
    {
        auto flagsSize = vmiInterface->getStructSizeFromJson(KernelStructOffsets::mmsection_flags::structName);
        assert((kernelOffsets.mmsectionFlags.file.endBit - kernelOffsets.mmsectionFlags.file.startBit) == 1);
        return static_cast<bool>(extractFlagValue(getMmSectionFlagsAddr(controlAreaBaseVA),
                                                  flagsSize,
                                                  kernelOffsets.mmsectionFlags.file.startBit,
                                                  kernelOffsets.mmsectionFlags.file.endBit));
    }

    addr_t KernelAccess::getVadNodeRightChildOffset() const
    {
        return kernelOffsets.mmVad.mmVadShortBaseAddress + kernelOffsets.mmVadShort.VadNode +
               kernelOffsets.rtlBalancedNode.Right;
    }

    addr_t KernelAccess::getVadNodeLeftChildOffset() const
    {
        return kernelOffsets.mmVad.mmVadShortBaseAddress + kernelOffsets.mmVadShort.VadNode +
               kernelOffsets.rtlBalancedNode.Left;
    }

    uint64_t KernelAccess::extractFlagValue(addr_t flagBaseVA, size_t size, size_t startBit, size_t endBit) const
    {
        expectSaneKernelAddress(flagBaseVA, static_cast<const char*>(__func__));
        uint64_t flagValue = 0;
        switch (size)
        {
            case sizeof(uint32_t):
                flagValue = vmiInterface->read32VA(flagBaseVA, vmiInterface->convertPidToDtb(systemPid));
                break;
            case sizeof(uint64_t):
                flagValue = vmiInterface->read64VA(flagBaseVA, vmiInterface->convertPidToDtb(systemPid));
                break;
            default:
                throw VmiException(fmt::format(
                    "{}: {} is unknown flag struct size", KernelStructOffsets::mmvad_flags::structName, size));
        }

        return getFlagValue(flagValue, startBit, endBit);
    }

    bool KernelAccess::extractIsWow64Process(uint64_t eprocessBase) const
    {
        auto wow64ProcessAddress = eprocessBase + kernelOffsets.eprocess.WoW64Process;
        auto wow64Process = vmiInterface->read64VA(wow64ProcessAddress, vmiInterface->convertPidToDtb(systemPid));

        return wow64Process != 0;
    }

    std::vector<uint32_t> KernelAccess::extractMmProtectToValue()
    {
        if (mmProtectToValue.has_value())
        {
            return mmProtectToValue.value();
        }

        constexpr std::size_t mmProtectToValueLength = 32;
        mmProtectToValue = std::vector<uint32_t>{};
        mmProtectToValue.value().reserve(mmProtectToValueLength);
        auto mmProtectToValueAddress = vmiInterface->translateKernelSymbolToVA("MmProtectToValue");

        for (std::size_t i = 0; i < mmProtectToValueLength; i++)
        {
            mmProtectToValue.value().push_back(vmiInterface->read32VA(mmProtectToValueAddress + i * sizeof(uint32_t),
                                                                      vmiInterface->convertPidToDtb(systemPid)));
        }

        return mmProtectToValue.value();
    }
}
