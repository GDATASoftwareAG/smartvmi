#include "FunctionDefinitions.h"
#include "../ConstantDefinitions.h"
#include "yaml-cpp/node/parse.h"
#include <fmt/core.h>

namespace ApiTracing
{
    FunctionDefinitions::FunctionDefinitions(std::filesystem::path functionDefinitionsPath)
        : functionDefinitionsPath(std::move(functionDefinitionsPath))
    {
    }

    void FunctionDefinitions::init()
    {
        auto rootNode = YAML::LoadFile(functionDefinitionsPath);
        if (!rootNode)
        {
            throw std::runtime_error(
                fmt::format("Could not load function definitions from {}.", functionDefinitionsPath.string()));
        }

        modulesNode = rootNode[NodeNames::moduleNodeName];
        if (!modulesNode)
        {
            throw std::runtime_error(
                fmt::format("Could not load module list. {} is undefined", NodeNames::moduleNodeName));
        }

        auto highLevelParameterTypesNode = rootNode[NodeNames::highLevelParameterTypesNodeName];
        if (!highLevelParameterTypesNode)
        {
            throw std::runtime_error(fmt::format("Could not load highLevelParameterTypesNode. {} is undefined",
                                                 NodeNames::highLevelParameterTypesNodeName));
        }

        highLevelParameterTypesNode32Bit = highLevelParameterTypesNode[NodeNames::highLevelParameterTypes32BitNodeName];
        if (!highLevelParameterTypesNode32Bit)
        {
            throw std::runtime_error(fmt::format("Could not load highLevelParameterTypesNode32Bit. {} is undefined",
                                                 NodeNames::highLevelParameterTypes32BitNodeName));
        }

        highLevelParameterTypesNode64Bit = highLevelParameterTypesNode[NodeNames::highLevelParameterTypes64BitNodeName];
        if (!highLevelParameterTypesNode64Bit)
        {
            throw std::runtime_error(fmt::format("Could not load highLevelParameterTypesNode64Bit. {} is undefined",
                                                 NodeNames::highLevelParameterTypes64BitNodeName));
        }

        backingParameterTypesNode = rootNode[NodeNames::backingParameterTypesNodeName];
        if (!backingParameterTypesNode)
        {
            throw std::runtime_error(fmt::format("Could not load backingParameterTypesNode. {} is undefined",
                                                 NodeNames::backingParameterTypesNodeName));
        }

        structuresNode = rootNode["Structures"];
        if (!structuresNode)
        {
            throw std::runtime_error(fmt::format("Could not load structuresNode. {} is undefined", "Structures"));
        }
    }

    std::shared_ptr<std::vector<ParameterInformation>> FunctionDefinitions::getFunctionParameterDefinitions(
        const std::string& moduleName, const std::string& functionName, uint16_t addressWidth)
    {
        auto moduleNode = modulesNode[moduleName];
        if (!moduleNode)
        {
            throw std::runtime_error(fmt::format("Requested module {} could not be found", moduleName));
        }

        auto functionNode = moduleNode[functionName];
        if (!functionNode)
        {
            throw std::runtime_error(
                fmt::format("Requested function {} could not be found in module {}", functionName, moduleName));
        }

        return std::make_shared<std::vector<ParameterInformation>>(
            getParameterInformation(functionNode["Parameters"], addressWidth));
    }

    std::vector<ParameterInformation>
    FunctionDefinitions::getParameterInformation(const YAML::Node& parameters, // NOLINT(misc-no-recursion)
                                                 uint16_t addressWidth)
    {
        std::vector<ParameterInformation> parameterInformationList;

        for (const auto parameter : parameters)
        {
            auto backingParameterName = parameter.first.as<std::string>();
            auto backingParameterType = parameter.second.as<std::string>();
            auto backingParameterSize = getParameterSize(backingParameterType, addressWidth);
            std::vector<ParameterInformation> backingParameters{};
            if (structuresNode[backingParameterType].IsDefined())
            {
                backingParameters = getStructParameterInformation(structuresNode[backingParameterType], addressWidth);
            }
            auto highLevelParameterNode = getHighLevelParameterTypesNode(addressWidth);
            auto backingBasicParameterType = getBasicParameterType(backingParameterType, highLevelParameterNode);

            parameterInformationList.emplace_back(
                backingBasicParameterType, backingParameterName, backingParameterSize, 0, backingParameters);
        }
        return parameterInformationList;
    }

    std::vector<ParameterInformation>
    FunctionDefinitions::getStructParameterInformation(const YAML::Node& parameters, // NOLINT(*-no-recursion)
                                                       uint16_t addressWidth)
    {
        std::vector<ParameterInformation> parameterInformationList;

        for (const auto parameter : parameters)
        {
            auto backingParameterName = parameter.first.as<std::string>();
            auto backingParameterType = parameter.second["Type"].as<std::string>();
            auto backingParameterSize = getParameterSize(backingParameterType, addressWidth);
            auto backingParameterOffset = parameter.second["Offset"].as<std::size_t>();
            auto backingParameters =
                structuresNode[backingParameterType].IsDefined()
                    ? getStructParameterInformation(structuresNode[backingParameterType], addressWidth)
                    : std::vector<ParameterInformation>{};
            auto highLevelParameterNode = getHighLevelParameterTypesNode(addressWidth);
            auto backingBasicParameterType = getBasicParameterType(backingParameterType, highLevelParameterNode);

            parameterInformationList.emplace_back(backingBasicParameterType,
                                                  backingParameterName,
                                                  backingParameterSize,
                                                  backingParameterOffset,
                                                  backingParameters);
        }
        return parameterInformationList;
    }

    uint8_t FunctionDefinitions::getParameterSize(const std::string& parameterType, uint16_t addressWidth)
    {
        auto parameterTypeCache = getParameterTypeCache(addressWidth);
        auto highLevelParameterTypesNode = getHighLevelParameterTypesNode(addressWidth);

        if (parameterTypeCache.contains(parameterType))
        {
            const auto& backingParameterType = parameterTypeCache.at(parameterType);
            return backingParameterTypesNode[backingParameterType].as<uint8_t>();
        }

        auto backingParameterType = getBasicParameterType(parameterType, highLevelParameterTypesNode);
        parameterTypeCache[parameterType] = backingParameterType;

        return backingParameterTypesNode[backingParameterType].as<uint8_t>();
    }

    std::map<std::string, std::string, std::less<>>& FunctionDefinitions::getParameterTypeCache(uint16_t addressWidth)
    {
        switch (addressWidth)
        {
            case ConstantDefinitions::x86AddressWidth:
            {
                return parameterSizeCache32Bit;
            }
            case ConstantDefinitions::x64AddressWidth:
            {
                return parameterSizeCache64Bit;
            }
            default:
            {
                throw std::logic_error(fmt::format("Unsupported address width {}.", addressWidth));
            }
        }
    }

    YAML::Node FunctionDefinitions::getHighLevelParameterTypesNode(uint16_t addressWidth) const
    {
        switch (addressWidth)
        {
            case ConstantDefinitions::x86AddressWidth:
            {
                return highLevelParameterTypesNode32Bit;
            }
            case ConstantDefinitions::x64AddressWidth:
            {
                return highLevelParameterTypesNode64Bit;
            }
            default:
            {
                throw std::logic_error(fmt::format("Unsupported address width {}.", addressWidth));
            }
        }
    }

    std::string
    FunctionDefinitions::getBasicParameterType(const std::string& parameterType, // NOLINT(misc-no-recursion)
                                               YAML::Node& highLevelParametersNode)
    {
        if (auto backingParameterType = backingParameterTypesNode[parameterType]; backingParameterType)
        {
            return parameterType;
        }

        if (auto structureType = structuresNode[parameterType]; structureType)
        {
            // Structs are represented as ptrs and we deduct the corresponding ptr length via the HANDLE type
            return getBasicParameterType("HANDLE", highLevelParametersNode);
        }
        auto highLevelParameterType = highLevelParametersNode[parameterType];
        if (!highLevelParameterType)
        {
            throw std::runtime_error(fmt::format("Could not find high level parameter type {}.", parameterType));
        }
        return getBasicParameterType(highLevelParameterType.as<std::string>(), highLevelParametersNode);
    }
}
