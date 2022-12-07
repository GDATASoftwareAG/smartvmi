#include "FunctionHook.h"
#include "Filenames.h"
#include <algorithm>
#include <fmt/core.h>
#include <utility>

using VmiCore::addr_t;
using VmiCore::BpResponse;
using VmiCore::IBreakpoint;
using VmiCore::IInterruptEvent;
using VmiCore::IIntrospectionAPI;
using VmiCore::Plugin::LogLevel;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{

    FunctionHook::FunctionHook(std::string moduleName,
                               std::string functionName,
                               std::shared_ptr<IExtractor> extractor,
                               std::shared_ptr<IIntrospectionAPI> introspectionAPI,
                               std::shared_ptr<std::vector<ParameterInformation>> parameterInformation,
                               PluginInterface* pluginInterface)
        : extractor(std::move(extractor)),
          introspectionAPI(std::move(introspectionAPI)),
          functionName(std::move(functionName)),
          moduleName(std::move(moduleName)),
          parameterInformation(std::move(parameterInformation)),
          pluginInterface(pluginInterface)
    {
    }

    void FunctionHook::hookFunction(addr_t moduleBaseAddress, uint64_t processCr3)
    {
        auto functionEntrypoint =
            introspectionAPI->translateUserlandSymbolToVA(moduleBaseAddress, processCr3, functionName);

        auto breakpointCallback = IBreakpoint::createBreakpointCallback(weak_from_this(), &FunctionHook::hookCallback);
        interruptEvent = pluginInterface->createBreakpoint(functionEntrypoint, processCr3, breakpointCallback);

        hookedProcesses.emplace_back(processCr3);
    }

    BpResponse FunctionHook::hookCallback(IInterruptEvent& event)
    {
        if (std::find(hookedProcesses.begin(), hookedProcesses.end(), event.getCr3()) != hookedProcesses.end())
        {
            pluginInterface->logMessage(
                LogLevel::info,
                LOG_FILENAME,
                fmt::format("hookCallback hit: {}->{} at VA{}", moduleName, functionName, event.getGla()));
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
        for (auto const& extractedParameter : extractedParameters)
        {
            std::visit(
                [extractedParameter = extractedParameter, this](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    using forward_type = std::remove_reference_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>)
                    {
                        return logParameter(extractedParameter.name, std::forward<forward_type>(arg));
                    }
                    else if constexpr (std::is_arithmetic_v<T>)
                    {
                        return logParameter(extractedParameter.name, std::to_string(std::forward<forward_type>(arg)));
                    }
                    else
                    {
                        throw std::runtime_error(
                            fmt::format("Can't convert extracted parameter {} to string.", extractedParameter.name));
                    }
                },
                extractedParameter.data);
            // TODO: Log backing parameters
        }
    }

    void FunctionHook::logParameter(const std::string& parameterName, const std::string& parameterValue) const
    {
        pluginInterface->logMessage(LogLevel::info, LOG_FILENAME, fmt::format("{}: {}", parameterName, parameterValue));
    }

    void FunctionHook::teardown() const
    {
        interruptEvent->remove();
    }
}
