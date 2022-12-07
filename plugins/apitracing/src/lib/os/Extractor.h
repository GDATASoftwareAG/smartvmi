#ifndef APITRACING_EXTRACTOR_H
#define APITRACING_EXTRACTOR_H

#include "../config/FunctionDefinitions.h"
#include <any>
#include <ostream>
#include <variant>
#include <vector>
#include <vmicore/vmi/IIntrospectionAPI.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace ApiTracing
{
    // NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
    enum class BasicTypes
    {
        INVALIC_BASIC_TYPE,
        LPSTR_32,
        LPSTR_64,
        LPWSTR_32,
        LPWSTR_64,
        UNICODE_WSTR_32,
        UNICODE_WSTR_64,
        __PTR32,
        __PTR64,
        INT,
        LONG,
        __INT64,
        UNSIGNED___INT64,
        UNSIGNED_LONG,
        UNSIGNED_INT,
        UNSIGNED_SHORT,
    };
    // NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

    struct ExtractedParameterInformation
    {
        std::string name;
        std::variant<std::string, uint64_t, int64_t> data;
        std::vector<ExtractedParameterInformation> backingParameters;

        bool operator==(const ExtractedParameterInformation& rhs) const = default;

        friend std::ostream& operator<<(std::ostream& os, const ExtractedParameterInformation& epi)
        {
            os << "name: \"" << epi.name << "\"";

            std::visit([&os](auto& arg) { os << ", data: \"" << arg << "\""; }, epi.data);

            if (!epi.backingParameters.empty())
            {
                os << ", backingParameters: [";
                for (const auto& p : epi.backingParameters)
                {
                    os << "{" << p << "}";
                }
                os << "]";
            }

            return os;
        }
    };

    class IExtractor
    {
      public:
        virtual ~IExtractor() = default;

        [[nodiscard]] virtual std::vector<ExtractedParameterInformation>
        extractParameters(VmiCore::IInterruptEvent& event,
                          const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation) = 0;

        [[nodiscard]] virtual std::vector<uint64_t>
        getShallowExtractedParams(VmiCore::IInterruptEvent& event,
                                  const std::shared_ptr<std::vector<ParameterInformation>>& parameterInformation) = 0;

        [[nodiscard]] virtual std::vector<ExtractedParameterInformation>
        getDeepExtractParameters(std::vector<uint64_t> shallowParameters,
                                 const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation,
                                 uint64_t cr3) = 0;

      protected:
        IExtractor() = default;
    };

    class Extractor : public IExtractor
    {
      public:
        Extractor(std::shared_ptr<VmiCore::IIntrospectionAPI> introspectionApi, uint8_t addressWidth);

        [[nodiscard]] std::vector<ExtractedParameterInformation>
        extractParameters(VmiCore::IInterruptEvent& event,
                          const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation) override;

        [[nodiscard]] std::vector<uint64_t> getShallowExtractedParams(
            VmiCore::IInterruptEvent& event,
            const std::shared_ptr<std::vector<ParameterInformation>>& parameterInformation) override;

        [[nodiscard]] std::vector<ExtractedParameterInformation>
        getDeepExtractParameters(std::vector<uint64_t> shallowParameters,
                                 const std::shared_ptr<std::vector<ParameterInformation>>& parametersInformation,
                                 uint64_t cr3) override;

      private:
        uint8_t addressWidth;
        std::variant<std::string, uint64_t, int> extractedParameters;
        std::shared_ptr<VmiCore::IIntrospectionAPI> introspectionAPI;
        std::map<std::string, BasicTypes, std::less<>> basicTypeStringToEnum{
            {"LPSTR_32", BasicTypes::LPSTR_32},
            {"LPSTR_64", BasicTypes::LPSTR_64},
            {"LPWSTR_32", BasicTypes::LPWSTR_32},
            {"LPWSTR_64", BasicTypes::LPWSTR_64},
            {"UNICODE_WSTR_32", BasicTypes::UNICODE_WSTR_32},
            {"UNICODE_WSTR_64", BasicTypes::UNICODE_WSTR_64},
            {"__ptr32", BasicTypes::__PTR32},
            {"__ptr64", BasicTypes::__PTR64},
            {"int", BasicTypes::INT},
            {"long", BasicTypes::LONG},
            {"__int64", BasicTypes::__INT64},
            {"unsigned __int64", BasicTypes::UNSIGNED___INT64},
            {"unsigned long", BasicTypes::UNSIGNED_LONG},
            {"unsigned int", BasicTypes::UNSIGNED_INT},
            {"unsigned short", BasicTypes::UNSIGNED_SHORT}};

        ExtractedParameterInformation extractSingleParameter(uint64_t shallowParameter,
                                                             uint64_t cr3,
                                                             const ParameterInformation& parameterInfo) const;

        [[nodiscard]] static std::vector<uint64_t> extractRegisterParameters(const VmiCore::IInterruptEvent& event,
                                                                             uint16_t registerParameterCount);

        [[nodiscard]] std::vector<uint64_t> extractStackParameters(
            uint64_t stackPointerVA, uint64_t cr3, std::vector<ParameterInformation> stackParameterInformation) const;

        [[nodiscard]] std::vector<ExtractedParameterInformation> extractBackingParameters(
            const std::vector<ParameterInformation>& backingParameters, uint64_t address, uint64_t cr3);

        [[nodiscard]] uint64_t readParameter(uint64_t stackPointerVA, uint8_t parameterSize, uint64_t cr3) const;

        [[nodiscard]] uint64_t zeroGarbageBytes(uint64_t parameter, uint8_t parameterSize) const;

        [[nodiscard]] std::string extractString(VmiCore::addr_t stringPointer, uint64_t cr3) const;

        [[nodiscard]] std::string extractUnicodeString(VmiCore::addr_t stringPointer, uint64_t cr3) const;

        [[nodiscard]] VmiCore::addr_t dereferencePointer(uint64_t addr, uint64_t cr3) const;
    };
}
#endif // APITRACING_EXTRACTOR_H
