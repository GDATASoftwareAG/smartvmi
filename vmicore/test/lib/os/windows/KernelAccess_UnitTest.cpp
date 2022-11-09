#include "../../vmi/ProcessesMemoryState.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <os/PagingDefinitions.h>

using testing::Contains;
using testing::Not;
using testing::StrEq;
using testing::UnorderedElementsAre;

namespace VmiCore
{
    class KernelAccessFixture : public ProcessesMemoryStateFixture
    {
        void SetUp() override
        {
            ProcessesMemoryStateFixture::SetUp();
            kernelAccess->initWindowsOffsets();
        }
    };

    TEST_F(KernelAccessFixture, extractFileName_ValidKernelspaceAddress_NoThrow)
    {
        EXPECT_CALL(*mockVmiInterface,
                    extractUnicodeStringAtVA(
                        PagingDefinitions::kernelspaceLowerBoundary + _FILE_OBJECT_OFFSETS::FileName, systemCR3))
            .Times(1);

        EXPECT_NO_THROW(auto filename = kernelAccess->extractFileName(PagingDefinitions::kernelspaceLowerBoundary));
    }

    TEST_F(KernelAccessFixture, extractFileName_MalformedKernelspaceAddress_Throws)
    {
        EXPECT_THROW(auto filename = kernelAccess->extractFileName(~PagingDefinitions::kernelspaceLowerBoundary),
                     std::invalid_argument);
    }
}
