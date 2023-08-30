#include "../src/lib/os/Extractor.h"
#include "ConstantDefinitions.h"
#include "TestConstantDefinitions.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore_test/vmi/mock_InterruptEvent.h>
#include <vmicore_test/vmi/mock_IntrospectionAPI.h>

using testing::ContainerEq;
using testing::Return;
using VmiCore::MockInterruptEvent;
using VmiCore::MockIntrospectionAPI;

namespace ApiTracing
{
    struct TestParameterInformation
    {
        ParameterInformation parameterInformation;
        uint64_t expectedValue;
    };

    namespace
    {
        constexpr uint64_t param1Value = 0xCAFEBABE;
        constexpr uint64_t param2Value = 0xDEADBEEF;
        constexpr uint64_t param3Value = 0xBBBB;
        constexpr uint64_t param4Value = 0xAA;
        constexpr uint64_t param5Value = 0xFEEBDAED;
        constexpr uint64_t param6Value = 0x0123456789101112;
        constexpr VmiCore::addr_t testDtb = 0x1337;
        constexpr VmiCore::addr_t testRsp = 0x420;
        constexpr uint64_t ObjectAttributesOneValue = 0xFFFFFFF1;
        constexpr uint64_t ObjectAttributesTwoValue = 0xFFFFFFF2;
        constexpr uint64_t ObjectAttributesTwoContentTwoValue = 0xFFFFFFF3;
        constexpr uint64_t ObjectAttributesTwoContentOneValue = 0x4711;
        constexpr uint64_t ObjectAttributesTwoContentTwoOffset = 0x8;
        constexpr uint64_t ObjectAttributesTwoContentOneOffset = 0x0;
        constexpr uint64_t ExtractedStringAddress = 0x420;

        const auto extractedString = "extract me";

        // clang-format off
        // @formatter:off
        const auto testParams64 = std::vector<TestParameterInformation>{
            {.parameterInformation{.basicType = "LPSTR_64", .name = "param1", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
             .expectedValue = param1Value},
            {.parameterInformation{.basicType = "int", .name = "param2", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
             .expectedValue = param2Value},
            {.parameterInformation{.basicType = "unsigned long", .name = "param3", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
             .expectedValue = param3Value},
            {.parameterInformation{.basicType = "__int64", .name = "param4", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
             .expectedValue = param4Value},
            {.parameterInformation{.basicType = "__ptr64", .name = "param5", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
             .expectedValue = param5Value},
            {.parameterInformation{.basicType = "UNICODE_WSTR_64", .name = "param6", .size = TestConstantDefinitions::eightBytes, .backingParameters{}},
             .expectedValue = param6Value},
        };
        // @formatter:on
        // clang-format on

        // clang-format off
        // @formatter:off
        const auto testParams32 = std::vector<TestParameterInformation>{

            {.parameterInformation{.name = "param1", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
             .expectedValue = param1Value},
            {.parameterInformation{.name = "param2", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
             .expectedValue = param2Value},
            {.parameterInformation{.name = "param3", .size = TestConstantDefinitions::twoBytes, .backingParameters{}},
             .expectedValue = param3Value},
            {.parameterInformation{.name = "param4", .size = TestConstantDefinitions::oneByte, .backingParameters{}},
             .expectedValue = param4Value},
            {.parameterInformation{.name = "param5", .size = TestConstantDefinitions::fourBytes, .backingParameters{}},
             .expectedValue = param5Value},
        };
        // @formatter:on
        // clang-format on

        const auto objectAttributesBackingParametersLevelTwo =
            std::vector<TestParameterInformation>{{.parameterInformation{.basicType = "unsigned long",
                                                                         .name = "ObjectAttributesTwoContentOne",
                                                                         .size = TestConstantDefinitions::fourBytes,
                                                                         .offset = ObjectAttributesTwoContentOneOffset,
                                                                         .backingParameters{}},
                                                   .expectedValue = ObjectAttributesTwoContentOneValue},
                                                  {.parameterInformation{.basicType = "LPSTR_64",
                                                                         .name = "ObjectAttributesTwoContentTwo",
                                                                         .size = TestConstantDefinitions::eightBytes,
                                                                         .offset = ObjectAttributesTwoContentTwoOffset,
                                                                         .backingParameters{}},
                                                   .expectedValue = ObjectAttributesTwoContentTwoValue}};

        const auto objectAttributesBackingParametersLevelOne = std::vector<TestParameterInformation>{
            {.parameterInformation{
                 .name = "ObjectAttributesTwo",
                 .size = TestConstantDefinitions::eightBytes,
                 .backingParameters{{objectAttributesBackingParametersLevelTwo.at(0).parameterInformation},
                                    {objectAttributesBackingParametersLevelTwo.at(1).parameterInformation}}},
             .expectedValue = ObjectAttributesTwoValue}};

        const auto testNestedStruct = std::vector<TestParameterInformation>{
            {.parameterInformation{.basicType = "unsigned __int64",
                                   .name = "FileHandle",
                                   .size = TestConstantDefinitions::eightBytes,
                                   .backingParameters{}},
             .expectedValue = param1Value},
            {.parameterInformation{
                 .name = "ObjectAttributesOne",
                 .size = TestConstantDefinitions::eightBytes,
                 .backingParameters{{objectAttributesBackingParametersLevelOne.at(0).parameterInformation}}},
             .expectedValue = ObjectAttributesOneValue}};
    }

    class ExtractorFixture : public testing::Test
    {
      protected:
        std::shared_ptr<MockIntrospectionAPI> introspectionAPI = std::make_shared<MockIntrospectionAPI>();
        std::shared_ptr<MockInterruptEvent> interruptEvent = std::make_shared<MockInterruptEvent>();
        std::shared_ptr<std::vector<ParameterInformation>> paramInformation;

        void SetUp() override
        {
            ON_CALL(*interruptEvent, getCr3()).WillByDefault(Return(testDtb));
            ON_CALL(*interruptEvent, getRcx).WillByDefault(Return(testParams64[0].expectedValue));
            ON_CALL(*interruptEvent, getRdx).WillByDefault(Return(testParams64[1].expectedValue));
            ON_CALL(*interruptEvent, getR8).WillByDefault(Return(testParams64[2].expectedValue));
            ON_CALL(*interruptEvent, getR9).WillByDefault(Return(testParams64[3].expectedValue));
            ON_CALL(*interruptEvent, getRsp).WillByDefault(Return(testRsp));
        }

        void SetupParameterInformation(const std::vector<TestParameterInformation>& testParameters)
        {
            paramInformation = std::make_shared<std::vector<ParameterInformation>>();
            for (const auto& parameter : testParameters)
            {
                paramInformation->emplace_back(parameter.parameterInformation);
            }
        }

        void SetupNestedStructPointerReads()
        {
            ON_CALL(*introspectionAPI, read64VA(param2Value, testDtb)).WillByDefault(Return(ObjectAttributesOneValue));

            ON_CALL(*introspectionAPI, read64VA(ObjectAttributesOneValue, testDtb))
                .WillByDefault(Return(ObjectAttributesTwoValue));

            ON_CALL(*introspectionAPI, readVA(ObjectAttributesTwoValue, testDtb, sizeof(uint32_t)))
                .WillByDefault(Return(ObjectAttributesTwoContentOneValue));
            ON_CALL(*introspectionAPI,
                    readVA(ObjectAttributesTwoValue + ObjectAttributesTwoContentTwoOffset, testDtb, sizeof(uint64_t)))
                .WillByDefault(Return(ExtractedStringAddress));

            ON_CALL(*introspectionAPI, extractStringAtVA(ExtractedStringAddress, testDtb))
                .WillByDefault([]() { return std::make_unique<std::string>(extractedString); });
        }

        std::vector<uint64_t> SetupParametersAndStack(const std::vector<TestParameterInformation>& parameters,
                                                      uint64_t addressWidth)
        {
            auto result = GetExpectedValues(parameters);
            SetupParameterInformation(parameters);

            if (addressWidth == ConstantDefinitions::x64AddressWidth)
            {
                SetupX64StackReads(parameters);
            }
            else
            {
                SetupX86StackReads(parameters);
            }
            return result;
        }

        void SetupX64StackReads(std::vector<TestParameterInformation> parameters)
        {
            auto stackEntrySize = ConstantDefinitions::x64AddressWidth / ConstantDefinitions::byteSize;
            auto stackOffset = 0;

            for (size_t i = ConstantDefinitions::maxRegisterParameterCount; i < parameters.size(); i++)
            {
                ON_CALL(*introspectionAPI,
                        read64VA(testRsp + ConstantDefinitions::stackParameterOffsetX64 + stackOffset, testDtb))
                    .WillByDefault(Return(parameters[i].expectedValue));
                stackOffset += stackEntrySize;
            }
        }

        void SetupX86StackReads(std::vector<TestParameterInformation> parameters)
        {
            for (size_t i = 0; i < parameters.size(); i++)
            {
                ON_CALL(*introspectionAPI,
                        read64VA(testRsp + (i + 1) * ConstantDefinitions::stackParameterOffsetX86, testDtb))
                    .WillByDefault(Return(parameters[i].expectedValue));
            }
        }

        static std::vector<ExtractedParameterInformation> SetupExpectedNestedParameters()
        {
            auto nestedStruct = ExtractedParameterInformation(
                {.name = "ObjectAttributesOne",
                 .data = {},
                 .backingParameters = std::vector<ExtractedParameterInformation>{ExtractedParameterInformation{
                     .name = "ObjectAttributesTwo",
                     .data = {},
                     .backingParameters = std::vector<ExtractedParameterInformation>{
                         ExtractedParameterInformation{.name = "ObjectAttributesTwoContentOne",
                                                       .data = ObjectAttributesTwoContentOneValue,
                                                       .backingParameters = {}},
                         ExtractedParameterInformation{.name = "ObjectAttributesTwoContentTwo",
                                                       .data = extractedString,
                                                       .backingParameters = {}}}}}});

            return std::vector<ExtractedParameterInformation>{
                ExtractedParameterInformation{
                    .name = "FileHandle", .data = static_cast<uint64_t>(param1Value), .backingParameters = {}},
                ExtractedParameterInformation{nestedStruct}};
        }

        static std::vector<uint64_t> GetExpectedValues(const std::vector<TestParameterInformation>& testParameters)
        {
            std::vector<uint64_t> expectedExtractedParameters{};
            for (const auto& testParameter : testParameters)
            {
                expectedExtractedParameters.emplace_back(testParameter.expectedValue);
            }
            return expectedExtractedParameters;
        }
    };

    TEST_F(ExtractorFixture, getShallowExtractedParams_64Bit0ParametersFunction_CorrectParametersExtracted)
    {
        auto extractor = std::make_shared<Extractor>(introspectionAPI, ConstantDefinitions::x64AddressWidth);
        auto expectedExtractedParameters = SetupParametersAndStack({}, ConstantDefinitions::x64AddressWidth);

        auto extractedParameters = extractor->getShallowExtractedParams(*interruptEvent, paramInformation);

        EXPECT_EQ(expectedExtractedParameters, extractedParameters);
    }

    TEST_F(ExtractorFixture, getShallowExtractedParams_32Bit0ParametersFunction_CorrectParametersExtracted)
    {
        auto extractor = std::make_shared<Extractor>(introspectionAPI, ConstantDefinitions::x86AddressWidth);
        auto expectedExtractedParameters = SetupParametersAndStack({}, ConstantDefinitions::x86AddressWidth);

        auto extractedParameters = extractor->getShallowExtractedParams(*interruptEvent, paramInformation);

        EXPECT_EQ(expectedExtractedParameters, extractedParameters);
    }

    TEST_F(ExtractorFixture, getShallowExtractedParams_32Bit4ParametersFunction_CorrectParametersExtracted)
    {
        auto extractor = std::make_shared<Extractor>(introspectionAPI, ConstantDefinitions::x86AddressWidth);
        auto expectedExtractedParameters = SetupParametersAndStack(testParams32, ConstantDefinitions::x86AddressWidth);

        auto extractedParameters = extractor->getShallowExtractedParams(*interruptEvent, paramInformation);

        EXPECT_EQ(expectedExtractedParameters, extractedParameters);
    }

    TEST_F(ExtractorFixture, getShallowExtractedParams_64Bit6ParametersFunction_CorrectParametersExtracted)
    {
        auto extractor = std::make_shared<Extractor>(introspectionAPI, ConstantDefinitions::x64AddressWidth);
        auto expectedExtractedParameters = SetupParametersAndStack(testParams64, ConstantDefinitions::x64AddressWidth);

        auto extractedParameters = extractor->getShallowExtractedParams(*interruptEvent, paramInformation);

        EXPECT_EQ(expectedExtractedParameters, extractedParameters);
    }

    TEST_F(ExtractorFixture, extractParameters_64Bit6ParametersFunction_CorrectParameters)
    {
        auto extractor = std::make_shared<Extractor>(introspectionAPI, ConstantDefinitions::x64AddressWidth);
        auto expectedExtractedParameters = SetupExpectedNestedParameters();
        auto relevantParameters = std::vector<TestParameterInformation>(testNestedStruct);
        SetupParameterInformation(relevantParameters);
        SetupNestedStructPointerReads();

        auto shallowParameters = extractor->getShallowExtractedParams(*interruptEvent, paramInformation);
        auto actualParameters = extractor->getDeepExtractParameters(shallowParameters, paramInformation, testDtb);

        ASSERT_EQ(actualParameters.size(), expectedExtractedParameters.size());
        EXPECT_THAT(expectedExtractedParameters, ContainerEq(actualParameters));
    }
}
