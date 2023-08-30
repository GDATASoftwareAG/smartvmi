#include "../src/lib/config/FunctionDefinitions.h"
#include "ConstantDefinitions.h"
#include "TestConstantDefinitions.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ContainerEq;

namespace ApiTracing
{
    class FunctionDefinitionsTestFixture : public testing::Test
    {
      protected:
        std::shared_ptr<FunctionDefinitions> functionDefinitions;

        void SetUp() override
        {
            functionDefinitions =
                std::make_shared<FunctionDefinitions>(std::filesystem::path("testFunctionDefinitions.yaml"));
            functionDefinitions->init();
        }

        // clang-format off
        // @formatter:off
        static std::vector<ParameterInformation> create64BitNtCreateFileFunctionDefinitions()
        {
            auto objectAttributesBackingParameters = std::vector<ParameterInformation>{
                {.basicType = "unsigned long", .name = "Length", .size = TestConstantDefinitions::fourBytes,.offset = 0, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "RootDirectory", .size = TestConstantDefinitions::eightBytes,.offset = 8, .backingParameters{}},
                {.basicType = "UNICODE_WSTR_64", .name = "ObjectName", .size = TestConstantDefinitions::eightBytes,.offset = 16, .backingParameters{}},
                {.basicType = "unsigned long", .name = "Attributes", .size = TestConstantDefinitions::fourBytes,.offset = 24, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "SecurityDescriptor", .size = TestConstantDefinitions::eightBytes,.offset = 32, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "SecurityQualityOfService", .size = TestConstantDefinitions::eightBytes,.offset = 40, .backingParameters{}}
            };
            return std::vector<ParameterInformation>{
                {.basicType = "unsigned __int64", .name = "FileHandle", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "DesiredAccess", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "ObjectAttributes", .size = TestConstantDefinitions::eightBytes, .backingParameters{objectAttributesBackingParameters}},
                {.basicType = "unsigned __int64", .name = "IoStatusBlock", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "AllocationSize", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "FileAttributes", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "ShareAccess", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "CreateDisposition", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "CreateOptions", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "EaBuffer", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "EaLength", .size = TestConstantDefinitions::fourBytes, .backingParameters{}}
            };
        }
        // @formatter:on
        // clang-format on
    };

    TEST_F(FunctionDefinitionsTestFixture, getFunctionParameterDefinitions_undefinedmodule_runtime_error)
    {
        EXPECT_THROW(auto ret = functionDefinitions->getFunctionParameterDefinitions(
                         "undefinedModule", "undefinedFunction", ConstantDefinitions::x64AddressWidth),
                     std::runtime_error);
    }

    TEST_F(FunctionDefinitionsTestFixture, getFunctionParameterDefinitions_undefinedFunction_runtime_error)
    {
        EXPECT_THROW(auto ret = functionDefinitions->getFunctionParameterDefinitions(
                         "crypt32.dll", "undefinedFunction", ConstantDefinitions::x64AddressWidth),
                     std::runtime_error);
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_validFunction32And64Bit_correctParameterInformation)
    {
        std::vector<ParameterInformation> expectedParameterInformation32Bit{{.basicType = "unsigned long",
                                                                             .name = "hProv",
                                                                             .size = TestConstantDefinitions::fourBytes,
                                                                             .backingParameters{}},
                                                                            {.basicType = "LPSTR_32",
                                                                             .name = "szSubsystemProtocol",
                                                                             .size = TestConstantDefinitions::fourBytes,
                                                                             .backingParameters{}}};
        std::vector<ParameterInformation> expectedParameterInformation64Bit{
            {.basicType = "unsigned long",
             .name = "hProv",
             .size = TestConstantDefinitions::fourBytes,
             .backingParameters{}},
            {.basicType = "LPSTR_64",
             .name = "szSubsystemProtocol",
             .size = TestConstantDefinitions::eightBytes,
             .backingParameters{}}};

        auto actualFunctionDefinitions32Bit = functionDefinitions->getFunctionParameterDefinitions(
            "crypt32.dll", "CertOpenSystemStoreA", ConstantDefinitions::x86AddressWidth);
        auto actualFunctionDefinitions64Bit = functionDefinitions->getFunctionParameterDefinitions(
            "crypt32.dll", "CertOpenSystemStoreA", ConstantDefinitions::x64AddressWidth);

        EXPECT_THAT(*actualFunctionDefinitions32Bit, ContainerEq(expectedParameterInformation32Bit));
        EXPECT_THAT(*actualFunctionDefinitions64Bit, ContainerEq(expectedParameterInformation64Bit));
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWith4Bytesize_correctParameterInformation)
    {
        std::vector<ParameterInformation> expectedParameterInformation{{.basicType = "unsigned long",
                                                                        .name = "dwMilliseconds",
                                                                        .size = TestConstantDefinitions::fourBytes,
                                                                        .backingParameters{}}};

        auto actualFunctionDefinitions = functionDefinitions->getFunctionParameterDefinitions(
            "KernelBase.dll", "Sleep", ConstantDefinitions::x64AddressWidth);

        EXPECT_THAT(*actualFunctionDefinitions, ContainerEq(expectedParameterInformation));
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWithTwoStepParameterTranslation_correctParameterInformation)
    {
        std::vector<ParameterInformation> expectedParameterInformation{{.basicType = "unsigned long",
                                                                        .name = "testParameter",
                                                                        .size = TestConstantDefinitions::fourBytes,
                                                                        .backingParameters{}}};

        auto actualFunctionDefinitions = functionDefinitions->getFunctionParameterDefinitions(
            "KernelBase.dll", "TestFunction", ConstantDefinitions::x64AddressWidth);

        EXPECT_THAT(*actualFunctionDefinitions, ContainerEq(expectedParameterInformation));
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWithMissingParameterTranslationStep_runtime_error)
    {
        EXPECT_THROW(auto ret = functionDefinitions->getFunctionParameterDefinitions(
                         "KernelBase.dll", "InvalidTestFunction", ConstantDefinitions::x64AddressWidth),
                     std::runtime_error);
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWithBackingParameters_correctParameterInformation)
    {
        auto expectedParameterInformation = create64BitNtCreateFileFunctionDefinitions();

        auto actualParameterDefinitions = functionDefinitions->getFunctionParameterDefinitions(
            "ntdll.dll", "NtCreateFile", ConstantDefinitions::x64AddressWidth);

        ASSERT_EQ(actualParameterDefinitions->size(), expectedParameterInformation.size());
        EXPECT_THAT(*actualParameterDefinitions, ContainerEq(expectedParameterInformation));
    }
}
