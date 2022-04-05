#ifndef VMICORE_KERNELOBJECTEXTRACTORWIN10_H
#define VMICORE_KERNELOBJECTEXTRACTORWIN10_H

#include "../../vmi/LibvmiInterface.h"
#include "KernelObjectDefinitionsWin10.h"

using namespace KernelObjectDefinitionsWin10;

class IKernelObjectExtractorWin10
{
  public:
    virtual ~IKernelObjectExtractorWin10() = default;

    virtual uint64_t getVadTreeRootAddress(uint64_t vadTreeRootNodeAddressLocation) const = 0;

    virtual uint64_t getImageFilePointer(uint64_t imageFilePointerAddressLocation) const = 0;

    virtual std::unique_ptr<_FILE_OBJECT> extractFileObject(uint64_t fileObjectBaseAddress) const = 0;

    virtual std::unique_ptr<_MMVAD_SHORT> extractMmVadShort(uint64_t vadEntryBaseVA) const = 0;

    virtual std::unique_ptr<_MMVAD> extractMmVad(uint64_t vadEntryBaseVA) const = 0;

    virtual std::unique_ptr<_SUBSECTION> extractMmVadSubsection(uint64_t subsectionAddress) const = 0;

    virtual std::unique_ptr<_CONTROL_AREA> extractControlArea(uint64_t controlAreaAddress) const = 0;

    virtual std::unique_ptr<std::string> extractWString(uint64_t bufferAddress, uint64_t stringLength) const = 0;

    virtual std::unique_ptr<_MMSECTION_FLAGS> extractMmSectionFlags(uint64_t sectionFlagsAddress) const = 0;

    virtual std::unique_ptr<_UNICODE_STRING> extractUnicodeObject(uint64_t unicodeObjectAddress) const = 0;

  protected:
    IKernelObjectExtractorWin10() = default;
};

class KernelObjectExtractorWin10 : public IKernelObjectExtractorWin10
{
  public:
    explicit KernelObjectExtractorWin10(const std::shared_ptr<ILibvmiInterface>& vmiInterface);

    ~KernelObjectExtractorWin10() override = default;

    uint64_t getVadTreeRootAddress(uint64_t vadTreeRootNodeAddressLocation) const override;

    uint64_t getImageFilePointer(uint64_t imageFilePointerAddressLocation) const override;

    std::unique_ptr<_FILE_OBJECT> extractFileObject(uint64_t fileObjectBaseAddress) const override;

    std::unique_ptr<_MMVAD_SHORT> extractMmVadShort(uint64_t vadEntryBaseVA) const override;

    std::unique_ptr<_MMVAD> extractMmVad(uint64_t vadEntryBaseVA) const override;

    std::unique_ptr<_SUBSECTION> extractMmVadSubsection(uint64_t subsectionAddress) const override;

    std::unique_ptr<_CONTROL_AREA> extractControlArea(uint64_t controlAreaAddress) const override;

    std::unique_ptr<std::string> extractWString(uint64_t bufferAddress, uint64_t stringLength) const override;

    std::unique_ptr<_MMSECTION_FLAGS> extractMmSectionFlags(uint64_t sectionFlagsAddress) const override;

    std::unique_ptr<_UNICODE_STRING> extractUnicodeObject(uint64_t unicodeObjectAddress) const override;

  private:
    std::shared_ptr<LibvmiInterface> vmiInterface;

    bool isSaneKernelAddress(uint64_t address) const;
};

#endif // VMICORE_KERNELOBJECTEXTRACTORWIN10_H
