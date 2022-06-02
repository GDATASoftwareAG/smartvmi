#include "KernelAccess.h"
#include "../PagingDefinitions.h"
#include <fmt/core.h>

namespace
{
    constexpr uint64_t exFastRefBits = 0xF;
}

KernelAccess::KernelAccess(std::shared_ptr<ILibvmiInterface> vmiInterface) : vmiInterface(std::move(vmiInterface)) {}

void KernelAccess::initWindowsOffsets()
{
    kernelOffsets = KernelOffsets::init(vmiInterface);
}

uint64_t KernelAccess::extractVadTreeRootAddress(uint64_t eprocessBase) const
{
    auto vadRoot = vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.VadRoot, vmiInterface->getSystemCr3());
    return vadRoot;
}

uint64_t KernelAccess::extractImageFilePointer(uint64_t eprocessBase) const
{
    auto imageFilePointer =
        vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.ImageFilePointer, vmiInterface->getSystemCr3());
    return imageFilePointer;
}

std::unique_ptr<std::string> KernelAccess::extractFileName(uint64_t fileObjectBaseAddress) const
{
    expectSaneKernelAddress(fileObjectBaseAddress, static_cast<const char*>(__func__));
    return vmiInterface->extractUnicodeStringAtVA(fileObjectBaseAddress + kernelOffsets.fileObject.FileName,
                                                  vmiInterface->getSystemCr3());
}

addr_t KernelAccess::extractControlAreaBasePointer(addr_t vadEntryBaseVA) const
{
    expectSaneKernelAddress(vadEntryBaseVA, static_cast<const char*>(__func__));
    auto subSectionBaseAddress =
        vmiInterface->read64VA(vadEntryBaseVA + kernelOffsets.mmVad.Subsection, vmiInterface->getSystemCr3());
    auto controlAreaBaseAddress = vmiInterface->read64VA(subSectionBaseAddress + kernelOffsets.subSection.ControlArea,
                                                         vmiInterface->getSystemCr3());
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
    auto filePointerObjectExFastRef = vmiInterface->read64VA(controlAreaBaseVA + kernelOffsets.controlArea.FilePointer +
                                                                 kernelOffsets.exFastRef.Object,
                                                             vmiInterface->getSystemCr3());
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
    auto leftChildAddress =
        vmiInterface->read64VA(currentVadEntryBaseVA + getVadNodeLeftChildOffset(), vmiInterface->getSystemCr3());
    auto rightChildAddress =
        vmiInterface->read64VA(currentVadEntryBaseVA + getVadNodeRightChildOffset(), vmiInterface->getSystemCr3());
    return {leftChildAddress, rightChildAddress};
}

std::tuple<uint64_t, uint64_t> KernelAccess::extractMmVadShortVpns(addr_t currentVadShortBaseVA) const
{
    expectSaneKernelAddress(currentVadShortBaseVA, static_cast<const char*>(__func__));
    auto startingVpnHigh = vmiInterface->read8VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.StartingVpnHigh,
                                                 vmiInterface->getSystemCr3());
    auto endingVpnHigh = vmiInterface->read8VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.EndingVpnHigh,
                                               vmiInterface->getSystemCr3());
    auto startingVpn = vmiInterface->read32VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.StartingVpn,
                                              vmiInterface->getSystemCr3());
    auto endingVpn = vmiInterface->read32VA(currentVadShortBaseVA + kernelOffsets.mmVadShort.EndingVpn,
                                            vmiInterface->getSystemCr3());
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint64_t vadShortEndingVpn = (static_cast<uint64_t>(endingVpnHigh) << sizeof(endingVpn) * 8) + endingVpn;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    uint64_t vadShortStartingVpn = (static_cast<uint64_t>(startingVpnHigh) << sizeof(startingVpn) * 8) + startingVpn;

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
                                  vmiInterface->getSystemCr3());
}

pid_t KernelAccess::extractParentID(addr_t eprocessBase) const
{
    return static_cast<pid_t>(vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.InheritedFromUniqueProcessId,
                                                     vmiInterface->getSystemCr3()));
}

std::string KernelAccess::extractImageFileName(addr_t eprocessBase) const
{
    return *vmiInterface->extractStringAtVA(eprocessBase + kernelOffsets.eprocess.ImageFileName,
                                            vmiInterface->getSystemCr3());
}

pid_t KernelAccess::extractPID(addr_t eprocessBase) const
{
    return static_cast<pid_t>(
        vmiInterface->read32VA(eprocessBase + kernelOffsets.eprocess.UniqueProcessId, vmiInterface->getSystemCr3()));
}

uint32_t KernelAccess::extractExitStatus(addr_t eprocessBase) const
{
    return vmiInterface->read32VA(eprocessBase + kernelOffsets.eprocess.ExitStatus, vmiInterface->getSystemCr3());
}

addr_t KernelAccess::extractSectionAddress(addr_t eprocessBase) const
{
    return vmiInterface->read64VA(eprocessBase + kernelOffsets.eprocess.SectionObject, vmiInterface->getSystemCr3());
}

addr_t KernelAccess::extractControlAreaAddress(addr_t sectionAddress) const
{
    expectSaneKernelAddress(sectionAddress, static_cast<const char*>(__func__));
    return vmiInterface->read64VA(sectionAddress + kernelOffsets.section.controlArea, vmiInterface->getSystemCr3());
}

addr_t KernelAccess::extractControlAreaFilePointer(addr_t controlAreaAddress) const
{
    expectSaneKernelAddress(controlAreaAddress, static_cast<const char*>(__func__));
    return vmiInterface->read64VA(controlAreaAddress + kernelOffsets.controlArea.FilePointer,
                                  vmiInterface->getSystemCr3());
}

std::unique_ptr<std::string> KernelAccess::extractProcessPath(addr_t filePointerAddress) const
{
    expectSaneKernelAddress(filePointerAddress, static_cast<const char*>(__func__));
    return vmiInterface->extractUnicodeStringAtVA(filePointerAddress + kernelOffsets.fileObject.FileName,
                                                  vmiInterface->getSystemCr3());
}

uint32_t KernelAccess::extractFlagValue32(addr_t flagBaseVA, size_t startBit, size_t endBit) const
{
    expectSaneKernelAddress(flagBaseVA, static_cast<const char*>(__func__));
    auto flagValue = vmiInterface->read32VA(flagBaseVA, vmiInterface->getSystemCr3());

    return getFlagValue(flagValue, startBit, endBit);
}

addr_t KernelAccess::getMmVadShortFlagsAddr(addr_t vadShortBaseVA) const
{
    return vadShortBaseVA + kernelOffsets.mmVadShort.Flags;
}

uint32_t KernelAccess::extractProtectionFlagValue(addr_t vadShortBaseVA) const
{
    return extractFlagValue32(getMmVadShortFlagsAddr(vadShortBaseVA),
                              kernelOffsets.mmvadFlags.protection.startBit,
                              kernelOffsets.mmvadFlags.protection.endBit);
}

bool KernelAccess::extractIsPrivateMemory(addr_t vadShortBaseVA) const
{
    return static_cast<bool>(extractFlagValue32(getMmVadShortFlagsAddr(vadShortBaseVA),
                                                kernelOffsets.mmvadFlags.privateMemory.startBit,
                                                kernelOffsets.mmvadFlags.privateMemory.endBit));
}

addr_t KernelAccess::getMmSectionFlagsAddr(addr_t controlAreaBaseVA) const
{
    return controlAreaBaseVA + kernelOffsets.controlArea._mmsection_flags;
}

bool KernelAccess::extractIsBeingDeleted(addr_t controlAreaBaseVA) const
{
    return static_cast<bool>(extractFlagValue32(getMmSectionFlagsAddr(controlAreaBaseVA),
                                                kernelOffsets.mmsectionFlags.beingDeleted.startBit,
                                                kernelOffsets.mmsectionFlags.beingDeleted.endBit));
}

bool KernelAccess::extractIsImage(addr_t controlAreaBaseVA) const
{
    return static_cast<bool>(extractFlagValue32(getMmSectionFlagsAddr(controlAreaBaseVA),
                                                kernelOffsets.mmsectionFlags.image.startBit,
                                                kernelOffsets.mmsectionFlags.image.endBit));
}

bool KernelAccess::extractIsFile(addr_t controlAreaBaseVA) const
{
    return static_cast<bool>(extractFlagValue32(getMmSectionFlagsAddr(controlAreaBaseVA),
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
