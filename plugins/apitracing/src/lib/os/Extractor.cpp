#include "Extractor.h"
#include "../ConstantDefinitions.h"
#include <fmt/core.h>

using VmiCore::addr_t;
using VmiCore::IInterruptEvent;
using VmiCore::IIntrospectionAPI;

namespace ApiTracing
{
    Extractor::Extractor(std::shared_ptr<IIntrospectionAPI> introspectionApi, uint8_t addressWidth)
        : addressWidth(addressWidth), introspectionAPI(std::move(introspectionApi))
    {
    }

    std::vector<ExtractedParameterInformation>
    Extractor::extractParameters(IInterruptEvent& event,
                                 const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation)
    {
        auto shallowExtractedParameters = getShallowExtractedParams(event, parametersInformation);
        auto deepExtractedParameters =
            getDeepExtractParameters(shallowExtractedParameters, parametersInformation, event.getCr3());
        return deepExtractedParameters;
    }

    std::vector<uint64_t>
    Extractor::getShallowExtractedParams(IInterruptEvent& event,
                                         const std::shared_ptr<std::vector<ParameterInformation>>& parameterInformation)
    {
        auto shallowExtractedParameters = std::vector<uint64_t>{};
        auto stackParams = std::vector<uint64_t>();
        auto registerParameterCount = static_cast<uint8_t>(std::min(
            parameterInformation->size(), static_cast<std::size_t>(ConstantDefinitions::maxRegisterParameterCount)));
        if (addressWidth == ConstantDefinitions::x64AddressWidth)
        {
            auto registerParameters = extractRegisterParameters(event, registerParameterCount);
            shallowExtractedParameters.insert(
                shallowExtractedParameters.begin(), registerParameters.begin(), registerParameters.end());
            // For x64 architecture shadowspace takes 0x20 bytes and first 4 parameters are passed in registers
            // There is no guarantee that the shadowspace will contain parameters for optimized programs
            // So we start reading the 5th parameter from rsp + 0x28
            if (parameterInformation->size() > ConstantDefinitions::maxRegisterParameterCount)
            {
                stackParams = extractStackParameters(
                    event.getRsp() + ConstantDefinitions::stackParameterOffsetX64,
                    event.getCr3(),
                    std::vector<ParameterInformation>(parameterInformation->begin() +
                                                          ConstantDefinitions::maxRegisterParameterCount,
                                                      parameterInformation->end()));
            }
        }
        else
        {
            stackParams = extractStackParameters(
                event.getRsp() + ConstantDefinitions::stackParameterOffsetX86, event.getCr3(), *parameterInformation);
            registerParameterCount = 0;
        }

        shallowExtractedParameters.insert(shallowExtractedParameters.begin() +
                                              static_cast<int64_t>(registerParameterCount),
                                          stackParams.begin(),
                                          stackParams.end());

        return shallowExtractedParameters;
    }

    std::vector<ExtractedParameterInformation> // NOLINTNEXTLINE (misc-no-recursion)
    Extractor::getDeepExtractParameters(std::vector<uint64_t> shallowParameters,
                                        const std::shared_ptr<std::vector<ParameterInformation>>& parameterInformation,
                                        uint64_t cr3)
    {
        std::vector<ExtractedParameterInformation> extractedParameterInformation{};
        uint8_t parameterIndex = 0;

        for (const auto& parameterInfo : *parameterInformation)
        {
            if (!parameterInfo.basicType.empty())
            {
                extractedParameterInformation.emplace_back(
                    extractSingleParameter(shallowParameters.at(parameterIndex), cr3, parameterInfo));
            }
            else if (!parameterInfo.backingParameters.empty())
            {
                addr_t structVA = dereferencePointer(shallowParameters.at(parameterIndex), cr3);
                ExtractedParameterInformation paramStruct{
                    .name = parameterInfo.name, .data = {}, .backingParameters = {}};
                paramStruct.backingParameters =
                    extractBackingParameters({parameterInfo.backingParameters}, structVA, cr3);
                extractedParameterInformation.emplace_back(paramStruct);
            }
            else
            {
                throw std::runtime_error("Malformed parameter information ! Aborting");
            }
            parameterIndex++;
        }
        return extractedParameterInformation;
    }

    ExtractedParameterInformation Extractor::extractSingleParameter(uint64_t shallowParameter,
                                                                    uint64_t cr3,
                                                                    const ParameterInformation& parameterInfo) const
    {
        ExtractedParameterInformation result{};
        result.name = parameterInfo.name;

        switch (basicTypeStringToEnum.at(parameterInfo.basicType))
        {
            using enum BasicTypes;

            case LPSTR_32:
            case LPSTR_64:
            {
                result.data = extractString(shallowParameter, cr3);
                break;
            }
            case LPWSTR_32:
            case LPWSTR_64:
            {
                result.data = extractWString(shallowParameter, cr3);
                break;
            }
            case UNICODE_WSTR_32:
            case UNICODE_WSTR_64:
            {
                result.data = extractUnicodeString(shallowParameter, cr3);
                break;
            }
            case __PTR32:
            case __PTR64:
            case UNSIGNED___INT64:
            case UNSIGNED_LONG:
            case UNSIGNED_INT:
            case UNSIGNED_SHORT:
            {
                result.data = shallowParameter;
                break;
            }
            case INT:
            case LONG:
            case __INT64:
            {
                result.data = static_cast<int64_t>(shallowParameter);
                break;
            }
            default:
            {
                break;
            }
        }
        return result;
    }

    std::vector<uint64_t> Extractor::extractRegisterParameters(const IInterruptEvent& event,
                                                               uint16_t registerParameterCount)
    {
        auto registerParams = std::vector<uint64_t>(registerParameterCount);
        switch (registerParameterCount)
        {
            case 4:
                registerParams[3] = event.getR9();
                [[fallthrough]];
            case 3:
                registerParams[2] = event.getR8();
                [[fallthrough]];
            case 2:
                registerParams[1] = event.getRdx();
                [[fallthrough]];
            case 1:
                registerParams[0] = event.getRcx();
                [[fallthrough]];
            case 0:
                break;
            default:
                throw std::invalid_argument(
                    fmt::format("Requested invalid amount of parameter extraction: {}", registerParameterCount));
        }
        return registerParams;
    }

    std::vector<uint64_t> Extractor::extractStackParameters(
        uint64_t stackPointerVA, uint64_t cr3, std::vector<ParameterInformation> stackParameterInformation) const
    {
        auto stackParams = std::vector<uint64_t>(stackParameterInformation.size());
        for (uint64_t i = 0; i < stackParameterInformation.size(); i++)
        {
            const auto& parameterType = stackParameterInformation.at(i);
            auto parameterLength = (parameterType.backingParameters.empty()) ? parameterType.size : addressWidth;
            auto parameter = readParameter(
                stackPointerVA + i * (addressWidth / ConstantDefinitions::byteSize), parameterLength, cr3);
            stackParams[i] = parameter;
        }

        return stackParams;
    }

    std::vector<ExtractedParameterInformation> // NOLINTNEXTLINE (misc-no-recursion)
    Extractor::extractBackingParameters(const std::vector<ParameterInformation>& backingParameters,
                                        uint64_t address,
                                        uint64_t cr3)
    {
        std::vector<ExtractedParameterInformation> extractedBackingParameters;
        for (const auto& parameter : backingParameters)
        {
            if (parameter.backingParameters.empty())
            {
                auto parameterValue = introspectionAPI->readVA(address + parameter.offset, cr3, parameter.size);
                extractedBackingParameters.emplace_back(extractSingleParameter(parameterValue, cr3, parameter));
            }
            else
            {
                // Extract shallow parameters (based on size) from shallowParameters.at(parameterIndex). Then pass those
                // to the nexţ iteration
                addr_t structPointer = dereferencePointer(address + parameter.offset, cr3);
                ExtractedParameterInformation extraction{.name = parameter.name, .data = {}, .backingParameters = {}};
                extraction.backingParameters =
                    extractBackingParameters(parameter.backingParameters, structPointer, cr3);
                extractedBackingParameters.emplace_back(extraction);
            }
        }
        return extractedBackingParameters;
    }

    uint64_t Extractor::readParameter(uint64_t stackPointerVA, uint8_t parameterSize, uint64_t cr3) const
    {
        auto output = introspectionAPI->read64VA(stackPointerVA, cr3);
        return zeroGarbageBytes(output, parameterSize);
    }

    uint64_t Extractor::zeroGarbageBytes(uint64_t parameter, uint8_t parameterSize) const
    {
        parameter = parameter << (addressWidth - parameterSize * ConstantDefinitions::byteSize);
        parameter = parameter >> (addressWidth - parameterSize * ConstantDefinitions::byteSize);
        return parameter;
    }

    std::string Extractor::extractString(addr_t stringPointer, uint64_t cr3) const
    {
        auto string = introspectionAPI->extractStringAtVA(stringPointer, cr3);
        return {*string};
    }

    std::string Extractor::extractWString(addr_t stringPointer, uint64_t cr3) const
    {
        auto string = introspectionAPI->extractWStringAtVA(stringPointer, cr3);
        return {*string};
    }

    std::string Extractor::extractUnicodeString(addr_t stringPointer, uint64_t cr3) const
    {
        auto string = introspectionAPI->extractUnicodeStringAtVA(stringPointer, cr3);
        return {*string};
    }

    addr_t Extractor::dereferencePointer(uint64_t addr, uint64_t cr3) const
    {
        if (addressWidth == ConstantDefinitions::x86AddressWidth)
        {
            return introspectionAPI->read32VA(addr, cr3);
        }
        // implicit x64 address width
        return introspectionAPI->read64VA(addr, cr3);
    }
}
