#include "FunctionHook.h"
#include "Filenames.h"
#include <fmt/core.h>
#include <utility>
#include <vmicore/callback.h>

using VmiCore::addr_t;
using VmiCore::BpResponse;
using VmiCore::IBreakpoint;
using VmiCore::IInterruptEvent;
using VmiCore::IIntrospectionAPI;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{

    FunctionHook::FunctionHook(std::string moduleName,
                               std::string functionName,
                               std::shared_ptr<IExtractor> extractor,
                               std::shared_ptr<VmiCore::IIntrospectionAPI> introspectionAPI,
                               std::shared_ptr<std::vector<ParameterInformation>> parameterInformation,
                               PluginInterface* pluginInterface)
        : extractor(std::move(extractor)),
          introspectionAPI(std::move(introspectionAPI)),
          functionName(std::move(functionName)),
          moduleName(std::move(moduleName)),
          parameterInformation(std::move(parameterInformation)),
          pluginInterface(pluginInterface),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
        builder["indentation"] = "";
    }

    void FunctionHook::hookFunction(VmiCore::addr_t moduleBaseAddress,
                                    std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation)
    {
        auto functionEntrypoint = introspectionAPI->translateUserlandSymbolToVA(
            moduleBaseAddress, processInformation->processUserDtb, functionName);

        breakpoint = pluginInterface->createBreakpoint(
            functionEntrypoint, *processInformation, VMICORE_SETUP_SAFE_MEMBER_CALLBACK(hookCallback));
    }

    BpResponse FunctionHook::hookCallback(IInterruptEvent& event)
    {
        logger->info(
            "hookCallback hit",
            {{"Module", moduleName}, {"Function", functionName}, {"Gla", fmt::format("{:x}", event.getGla())}});
        if (parameterInformation->empty())
        {
            return BpResponse::Continue;
        }

        auto extractedParameters = extractor->extractParameters(event, parameterInformation);
        auto json = getParameterListAsJson(extractedParameters);
        std::string unformattedTraces = Json::writeString(builder, json);

        logger->info("Monitored function called",
                     {{"FunctionName", functionName},
                      {"ModuleName", moduleName},
                      {"ProcessDtb", fmt::format("{:x}", event.getCr3())},
                      {"ProcessTeb", fmt::format("{:x}", event.getGs())},
                      {"Parameterlist", unformattedTraces}});

        return BpResponse::Continue;
    }

    Json::Value
    FunctionHook::getParameterListAsJson(const std::vector<ExtractedParameterInformation>& extractedParameters)
    {
        Json::Value parameterList;

        for (const auto& extractedParameter : extractedParameters)
        {
            Json::Value parameter;
            if (!extractedParameter.backingParameters.empty())
            {
                parameter[extractedParameter.name] = getParameterListAsJson(extractedParameter.backingParameters);
            }
            else
            {
                std::visit([&parameter = parameter, &extractedParameter = extractedParameter]<typename T>(T&& arg)
                           { parameter[extractedParameter.name] = std::forward<T>(arg); },
                           extractedParameter.data);
            }
            parameterList.append(parameter);
        }
        return parameterList;
    }

    void FunctionHook::teardown() const
    {
        breakpoint->remove();
    }
}
