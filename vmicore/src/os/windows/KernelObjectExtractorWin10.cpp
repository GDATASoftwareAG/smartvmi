#include "KernelObjectExtractorWin10.h"
#include "../PagingDefinitions.h"

KernelObjectExtractorWin10::KernelObjectExtractorWin10(const std::shared_ptr<ILibvmiInterface>& vmiInterface)
    : vmiInterface(std::dynamic_pointer_cast<LibvmiInterface>(vmiInterface))
{
}

uint64_t KernelObjectExtractorWin10::getVadTreeRootAddress(uint64_t vadTreeRootNodeAddressLocation) const
{
    if (!isSaneKernelAddress(vadTreeRootNodeAddressLocation))
    {
        throw std::invalid_argument(std::string(__func__) + ": " +
                                    Convenience::intToHex(vadTreeRootNodeAddressLocation) +
                                    " is not a kernel space address");
    }
    return vmiInterface->read64VA(vadTreeRootNodeAddressLocation, vmiInterface->getSystemCr3());
}

uint64_t KernelObjectExtractorWin10::getImageFilePointer(uint64_t imageFilePointerAddressLocation) const
{
    if (!isSaneKernelAddress(imageFilePointerAddressLocation))
    {
        throw std::invalid_argument(std::string(__func__) + ": " +
                                    Convenience::intToHex(imageFilePointerAddressLocation) +
                                    " is not a kernel space address");
    }
    return vmiInterface->read64VA(imageFilePointerAddressLocation, vmiInterface->getSystemCr3());
}

std::unique_ptr<_FILE_OBJECT> KernelObjectExtractorWin10::extractFileObject(uint64_t fileObjectBaseAddress) const
{
    if (!isSaneKernelAddress(fileObjectBaseAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(fileObjectBaseAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_FILE_OBJECT>(fileObjectBaseAddress, vmiInterface->getSystemCr3());
}

std::unique_ptr<_MMVAD_SHORT> KernelObjectExtractorWin10::extractMmVadShort(uint64_t vadEntryBaseVA) const
{
    if (!isSaneKernelAddress(vadEntryBaseVA))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(vadEntryBaseVA) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_MMVAD_SHORT>(vadEntryBaseVA, vmiInterface->getSystemCr3());
}

std::unique_ptr<_MMVAD> KernelObjectExtractorWin10::extractMmVad(uint64_t vadEntryBaseVA) const
{
    if (!isSaneKernelAddress(vadEntryBaseVA))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(vadEntryBaseVA) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_MMVAD>(vadEntryBaseVA, vmiInterface->getSystemCr3());
}

std::unique_ptr<_SUBSECTION> KernelObjectExtractorWin10::extractMmVadSubsection(uint64_t subsectionAddress) const
{
    if (!isSaneKernelAddress(subsectionAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(subsectionAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_SUBSECTION>(subsectionAddress, vmiInterface->getSystemCr3());
}

std::unique_ptr<_CONTROL_AREA> KernelObjectExtractorWin10::extractControlArea(uint64_t controlAreaAddress) const
{
    if (!isSaneKernelAddress(controlAreaAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(controlAreaAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_CONTROL_AREA>(controlAreaAddress, vmiInterface->getSystemCr3());
}

std::unique_ptr<std::string> KernelObjectExtractorWin10::extractWString(uint64_t bufferAddress,
                                                                        uint64_t stringLength) const
{
    if (!isSaneKernelAddress(bufferAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(bufferAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->extractWStringAtVA(bufferAddress, stringLength, vmiInterface->getSystemCr3());
}

std::unique_ptr<_MMSECTION_FLAGS> KernelObjectExtractorWin10::extractMmSectionFlags(uint64_t sectionFlagsAddress) const
{
    if (!isSaneKernelAddress(sectionFlagsAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(sectionFlagsAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_MMSECTION_FLAGS>(sectionFlagsAddress, vmiInterface->getSystemCr3());
}

std::unique_ptr<_UNICODE_STRING> KernelObjectExtractorWin10::extractUnicodeObject(uint64_t unicodeObjectAddress) const
{
    if (!isSaneKernelAddress(unicodeObjectAddress))
    {
        throw std::invalid_argument(std::string(__func__) + ": " + Convenience::intToHex(unicodeObjectAddress) +
                                    " is not a kernel space address");
    }
    return vmiInterface->readVa<_UNICODE_STRING>(unicodeObjectAddress, vmiInterface->getSystemCr3());
}

bool KernelObjectExtractorWin10::isSaneKernelAddress(uint64_t address) const
{
    return address >= PagingDefinitions::kernelspaceLowerBoundary;
}