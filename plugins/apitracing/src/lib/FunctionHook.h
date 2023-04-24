#ifndef APITRACING_FUNCTIONHOOK_H
#define APITRACING_FUNCTIONHOOK_H

#include "ConstantDefinitions.h"
#include "config/FunctionDefinitions.h"
#include "os/Extractor.h"
#include <vmicore/io/ILogger.h>
#include <vmicore/plugins/PluginInterface.h>
#include <vmicore/vmi/IBreakpoint.h>

namespace ApiTracing
{
    class FunctionHook : public std::enable_shared_from_this<FunctionHook>
    {
      public:
        FunctionHook(std::string moduleName,
                     std::string functionName,
                     std::shared_ptr<IExtractor> extractor,
                     std::shared_ptr<VmiCore::IIntrospectionAPI> introspectionAPI,
                     std::shared_ptr<std::vector<ParameterInformation>> parameterInformation,
                     VmiCore::Plugin::PluginInterface* pluginInterface);

        void hookFunction(VmiCore::addr_t moduleBaseAddress, uint64_t processCr3);

        VmiCore::BpResponse hookCallback(VmiCore::IInterruptEvent& event);

        void teardown() const;

      private:
        std::shared_ptr<IExtractor> extractor;
        std::shared_ptr<VmiCore::IBreakpoint> interruptEvent;
        std::shared_ptr<VmiCore::IIntrospectionAPI> introspectionAPI;
        std::string functionName;
        std::vector<uint64_t> hookedProcesses;
        std::string moduleName;
        std::shared_ptr<std::vector<ParameterInformation>> parameterInformation;
        VmiCore::Plugin::PluginInterface* pluginInterface;
        std::unique_ptr<VmiCore::ILogger> logger;

        void logParameterList(const std::vector<ExtractedParameterInformation>& extractedParameters);
    };
}
#endif // APITRACING_FUNCTIONHOOK_H
