#include "../../../src/os/windows/KernelObjectExtractorWin10.h"
#include <gmock/gmock.h>

class MockKernelObjectExtractorWin10 : public IKernelObjectExtractorWin10
{
  public:
    MOCK_CONST_METHOD1(getVadTreeRootAddress, uint64_t(uint64_t vadTreeRootNodeAddressLocation));

    MOCK_CONST_METHOD1(getImageFilePointer, uint64_t(uint64_t imageFilePointerAddressLocation));

    MOCK_CONST_METHOD1(extractFileObject, std::unique_ptr<_FILE_OBJECT>(uint64_t fileObjectBaseAddress));

    MOCK_CONST_METHOD1(extractMmVadShort, std::unique_ptr<_MMVAD_SHORT>(uint64_t vadEntryBaseVA));

    MOCK_CONST_METHOD1(extractMmVad, std::unique_ptr<_MMVAD>(uint64_t vadEntryBaseVA));

    MOCK_CONST_METHOD1(extractMmVadSubsection, std::unique_ptr<_SUBSECTION>(uint64_t subsection));

    MOCK_CONST_METHOD1(extractControlArea, std::unique_ptr<_CONTROL_AREA>(uint64_t controlArea));

    MOCK_CONST_METHOD2(extractWString, std::unique_ptr<std::string>(uint64_t bufferAddress, uint64_t stringLength));

    MOCK_CONST_METHOD1(extractMmSectionFlags, std::unique_ptr<_MMSECTION_FLAGS>(uint64_t sectionFlagsAddress));

    MOCK_CONST_METHOD1(extractUnicodeObject, std::unique_ptr<_UNICODE_STRING>(uint64_t unicodeObjectAddress));
};
