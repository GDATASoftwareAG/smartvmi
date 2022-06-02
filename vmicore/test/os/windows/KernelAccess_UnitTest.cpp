#include "../../src/os/PagingDefinitions.h"
#include "../../vmi/ProcessesMemoryState.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Contains;
using testing::Not;
using testing::StrEq;
using testing::UnorderedElementsAre;

class KernelAccessFixture : public ProcessesMemoryStateFixture
{
    void SetUp() override
    {
        ProcessesMemoryStateFixture::SetUp();
    }
};

TEST_F(KernelAccessFixture, extractFileName_ValidKernelspaceAddress_NoThrow)
{
    EXPECT_CALL(*mockVmiInterface,
                extractUnicodeStringAtVA(
                    PagingDefinitions::kernelspaceLowerBoundary + kernelOffsets->fileObject.FileName, systemCR3))
        .Times(1);

    EXPECT_NO_THROW(auto filename = kernelAccess->extractFileName(PagingDefinitions::kernelspaceLowerBoundary));
}

TEST_F(KernelAccessFixture, extractFileName_MalformedKernelspaceAddress_Throws)
{
    EXPECT_THROW(auto filename = kernelAccess->extractFileName(~PagingDefinitions::kernelspaceLowerBoundary),
                 std::invalid_argument);
}
