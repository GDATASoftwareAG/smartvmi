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
                {{.basicType = "unsigned long", .name = "Length", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                 {.basicType = "unsigned __int64", .name = "RootDirectory", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                 {.basicType = "UNICODE_WSTR_64", .name = "ObjectName", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                 {.basicType = "unsigned long", .name = "Attributes", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                 {.basicType = "unsigned __int64", .name = "SecurityDescriptor", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                 {.basicType = "unsigned __int64", .name = "SecurityQualityOfService", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}}}
            };
            return std::vector<ParameterInformation>{
                {.basicType = "unsigned __int64", .name = "FileHandle", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "DesiredAccess", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "ObjectAttributes", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{objectAttributesBackingParameters}},
                {.basicType = "unsigned __int64", .name = "IoStatusBlock", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "AllocationSize", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "FileAttributes", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "ShareAccess", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "CreateDisposition", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "CreateOptions", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}},
                {.basicType = "unsigned __int64", .name = "EaBuffer", .parameterSize = TestConstantDefinitions::eightBytes, .backingParameters{}},
                {.basicType = "unsigned long", .name = "EaLength", .parameterSize = TestConstantDefinitions::fourBytes, .backingParameters{}}};
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
        std::vector<ParameterInformation> expectedParameterInformation32Bit{
            {.basicType = "unsigned long",
             .name = "hProv",
             .parameterSize = TestConstantDefinitions::fourBytes,
             .backingParameters{}},
            {.basicType = "LPSTR_32",
             .name = "szSubsystemProtocol",
             .parameterSize = TestConstantDefinitions::fourBytes,
             .backingParameters{}}};
        std::vector<ParameterInformation> expectedParameterInformation64Bit{
            {.basicType = "unsigned long",
             .name = "hProv",
             .parameterSize = TestConstantDefinitions::fourBytes,
             .backingParameters{}},
            {.basicType = "LPSTR_64",
             .name = "szSubsystemProtocol",
             .parameterSize = TestConstantDefinitions::eightBytes,
             .backingParameters{}}};

        auto actualFunctionDefinitions32Bit = functionDefinitions->getFunctionParameterDefinitions(
            "crypt32.dll", "CertOpenSystemStoreA", ConstantDefinitions::x86AddressWidth);
        auto actualFunctionDefinitions64Bit = functionDefinitions->getFunctionParameterDefinitions(
            "crypt32.dll", "CertOpenSystemStoreA", ConstantDefinitions::x64AddressWidth);

        EXPECT_THAT(*actualFunctionDefinitions32Bit, ContainerEq(expectedParameterInformation32Bit));
        EXPECT_THAT(*actualFunctionDefinitions64Bit, ContainerEq(expectedParameterInformation64Bit));
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWith4ByteParameterSize_correctParameterInformation)
    {
        std::vector<ParameterInformation> expectedParameterInformation{
            {.basicType = "unsigned long",
             .name = "dwMilliseconds",
             .parameterSize = TestConstantDefinitions::fourBytes,
             .backingParameters{}}};

        auto actualFunctionDefinitions = functionDefinitions->getFunctionParameterDefinitions(
            "KernelBase.dll", "Sleep", ConstantDefinitions::x64AddressWidth);

        EXPECT_THAT(*actualFunctionDefinitions, ContainerEq(expectedParameterInformation));
    }

    TEST_F(FunctionDefinitionsTestFixture,
           getFunctionParameterDefinitions_functionWithTwoStepParameterTranslation_correctParameterInformation)
    {
        std::vector<ParameterInformation> expectedParameterInformation{
            {.basicType = "unsigned long",
             .name = "testParameter",
             .parameterSize = TestConstantDefinitions::fourBytes,
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
