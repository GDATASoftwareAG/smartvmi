#include "FunctionHook.h"
#include "Filenames.h"
#include <algorithm>
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
    }

    void FunctionHook::hookFunction(VmiCore::addr_t moduleBaseAddress,
                                    std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation)
    {
        auto processDtb = moduleBaseAddress >= 0xFFFF800000000000 ? processInformation->processKernelDTB
                                                                  : processInformation->processUserDTB;

        auto functionEntrypoint =
            introspectionAPI->translateUserlandSymbolToVA(moduleBaseAddress, processDtb, functionName);

        breakpoint = pluginInterface->createBreakpoint(
            functionEntrypoint, processInformation, VMICORE_SETUP_SAFE_MEMBER_CALLBACK(hookCallback));

        hookedProcesses.emplace_back(processDtb);
    }

    BpResponse FunctionHook::hookCallback(IInterruptEvent& event)
    {
        if (std::find(hookedProcesses.begin(), hookedProcesses.end(), event.getCr3()) != hookedProcesses.end())
        {
            logger->info(
                "hookCallback hit",
                {{"Module", moduleName}, {"Function", functionName}, {"Gla", fmt::format("{:x}", event.getGla())}});
            if (parameterInformation->empty())
            {
                return BpResponse::Continue;
            }

            auto extractedParameters = extractor->extractParameters(event, parameterInformation);
            logParameterList(extractedParameters);
        }
        return BpResponse::Continue;
    }

    void FunctionHook::logParameterList(const std::vector<ExtractedParameterInformation>& extractedParameters)
    {
        for (const auto& extractedParameter : extractedParameters)
        {
            std::visit(
                [&extractedParameter = extractedParameter, &logger = logger](auto&& arg)
                {
                    logger->info("Parameter",
                                 {{"Name", extractedParameter.name},
                                  {"Value", std::forward<std::remove_reference_t<decltype(arg)>>(arg)}});
                },
                extractedParameter.data);
            // TODO: Log backing parameters
        }
    }

    void FunctionHook::teardown() const
    {
        breakpoint->remove();
    }
}
