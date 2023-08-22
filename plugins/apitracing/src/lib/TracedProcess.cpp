#include "TracedProcess.h"
#include "ConstantDefinitions.h"
#include "Filenames.h"
#include "FunctionHook.h"
#include "os/Extractor.h"

#include <utility>

namespace ApiTracing
{
    TracedProcess::TracedProcess(VmiCore::Plugin::PluginInterface* pluginInterface,
                                 std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                                 std::shared_ptr<ILibrary> library,
                                 std::shared_ptr<const VmiCore::ActiveProcessInformation> processInformation,
                                 TracingProfile tracingProfile)
        : pluginInterface(pluginInterface),
          functionDefinitions(std::move(functionDefinitions)),
          library(std::move(library)),
          processInformation(std::move(processInformation)),
          tracingProfile(std::move(tracingProfile)),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        initLoadedModules();
        injectHooks();
    }

    void TracedProcess::removeHooks() noexcept
    {
        for (const auto& hook : hookList)
        {
            try
            {
                hook->teardown();
            }
            catch (const std::exception& e)
            {
                logger->warning("Unable to remove hook", {{"Exception", e.what()}});
            }
        }

        hookList.clear();
    }

    bool TracedProcess::traceChildren() const
    {
        return tracingProfile.traceChildren;
    }

    TracingProfile TracedProcess::getTracingProfile()
    {
        return tracingProfile;
    }

    void TracedProcess::initLoadedModules()
    {
        auto memoryRegions = processInformation->memoryRegionExtractor->extractAllMemoryRegions();

        for (const auto& memoryRegionDescriptor : *memoryRegions)
        {
            auto filename = library->splitFilenameFromRegionName(memoryRegionDescriptor.moduleName);

            if ((library->isTraceableLibrary(memoryRegionDescriptor.moduleName)) &&
                (!loadedModules.contains(*filename)))
            {
                loadedModules.emplace(*filename, memoryRegionDescriptor.base);
            }
        }
    }

    void TracedProcess::injectHooks()
    {
        auto introspectionAPI = pluginInterface->getIntrospectionAPI();

        hookList.reserve(numberOfFunctionsToTrace());

        for (const auto& moduleHookTarget : tracingProfile.modules)
        {
            auto moduleBaseAddress = loadedModules[moduleHookTarget.name];
            if (moduleBaseAddress == 0)
            {
                continue;
            }

            for (const auto& functionName : moduleHookTarget.functions)
            {
                try
                {
                    auto addressWidth = processInformation->is32BitProcess ? ConstantDefinitions::x86AddressWidth
                                                                           : ConstantDefinitions::x64AddressWidth;
                    auto definitions = functionDefinitions->getFunctionParameterDefinitions(
                        moduleHookTarget.name, functionName, addressWidth);
                    auto extractor = std::make_shared<Extractor>(introspectionAPI, addressWidth);
                    auto functionHook = std::make_shared<FunctionHook>(
                        moduleHookTarget.name, functionName, extractor, introspectionAPI, definitions, pluginInterface);
                    functionHook->hookFunction(moduleBaseAddress, processInformation);
                    hookList.push_back(functionHook);
                }
                catch (const std::exception& e)
                {
                    logger->warning("Could not trace function",
                                    {{"Library", moduleHookTarget.name},
                                     {"Function", functionName},
                                     {"Process", processInformation->name},
                                     {"Pid", processInformation->pid},
                                     {"Exception", e.what()}});
                }
            }
        }

        hookList.shrink_to_fit();
    }

    std::size_t TracedProcess::numberOfFunctionsToTrace() const
    {
        std::size_t numberOfFunctions = 0;
        for (const auto& el : tracingProfile.modules)
        {
            numberOfFunctions += el.functions.size();
        }
        return numberOfFunctions;
    }
}
