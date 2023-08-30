#ifndef APITRACING_FUNCTIONDEFINITIONS_H
#define APITRACING_FUNCTIONDEFINITIONS_H

#include <filesystem>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <yaml-cpp/yaml.h>

namespace ApiTracing
{
    struct ParameterInformation
    {
        std::string basicType{};
        std::string name{};
        uint8_t size{};
        std::size_t offset{};
        std::vector<ParameterInformation> backingParameters{};

        bool operator==(const ParameterInformation& rhs) const = default;

        friend std::ostream& operator<<(std::ostream& os, const ParameterInformation& pi)
        {
            os << "basicType: \"" << pi.basicType << "\"";
            os << ", name: \"" << pi.name << "\"";
            os << ", parameterSize: \"" << pi.size << "\"";

            if (!pi.backingParameters.empty())
            {
                os << ", backingParameters: [";
                for (const auto& p : pi.backingParameters)
                {
                    os << "{" << p << "}";
                }
                os << "]";
            }

            return os;
        }
    };

    namespace NodeNames
    {
        constexpr const char* moduleNodeName = "Modules";
        constexpr const char* highLevelParameterTypesNodeName = "HighLevelParameterTypes";
        constexpr const char* highLevelParameterTypes32BitNodeName = "AddressWidth32Bit";
        constexpr const char* highLevelParameterTypes64BitNodeName = "AddressWidth64Bit";
        constexpr const char* backingParameterTypesNodeName = "BackingParameterTypes";
    }

    class IFunctionDefinitions
    {
      public:
        virtual ~IFunctionDefinitions() = default;

        [[nodiscard]] virtual std::shared_ptr<std::vector<ParameterInformation>> getFunctionParameterDefinitions(
            const std::string& moduleName, const std::string& functionName, uint16_t addressWidth) = 0;

        virtual void init() = 0;

      protected:
        IFunctionDefinitions() = default;
    };

    class FunctionDefinitions : public IFunctionDefinitions
    {
      public:
        explicit FunctionDefinitions(std::filesystem::path functionDefinitionsPath);

        void init() override;

        [[nodiscard]] std::shared_ptr<std::vector<ParameterInformation>> getFunctionParameterDefinitions(
            const std::string& moduleName, const std::string& functionName, uint16_t addressWidth) override;

      private:
        YAML::Node modulesNode;
        YAML::Node structuresNode;
        YAML::Node highLevelParameterTypesNode32Bit;
        YAML::Node highLevelParameterTypesNode64Bit;
        YAML::Node backingParameterTypesNode;
        std::filesystem::path functionDefinitionsPath;

        std::map<std::string, std::string, std::less<>> parameterSizeCache32Bit{};
        std::map<std::string, std::string, std::less<>> parameterSizeCache64Bit{};

        uint8_t getParameterSize(const std::string& parameterType, uint16_t addressWidth);
        std::string getBasicParameterType(const std::string& parameterType, YAML::Node& highLevelParametersNode);
        YAML::Node getHighLevelParameterTypesNode(uint16_t addressWidth) const;
        std::map<std::string, std::string, std::less<>>& getParameterTypeCache(uint16_t addressWidth);
        std::vector<ParameterInformation> getParameterInformation(const YAML::Node& parameters, uint16_t addressWidth);
        std::vector<ParameterInformation> getStructParameterInformation(const YAML::Node& parameters,
                                                                        uint16_t addressWidth);
    };
}
#endif // APITRACING_FUNCTIONDEFINITIONS_H
