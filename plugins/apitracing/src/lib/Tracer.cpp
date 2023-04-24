#include "Tracer.h"
#include "Config.h"
#include "Filenames.h"
#include <filesystem>
#include <vector>

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    Tracer::Tracer(PluginInterface* pluginInterface,
                   std::shared_ptr<IConfig> configuration,
                   std::shared_ptr<std::vector<ProcessInformation>> tracingInformation,
                   std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                   std::shared_ptr<ILibrary> library)
        : pluginInterface(pluginInterface),
          configuration(std::move(configuration)),
          tracingTargetsInformation(std::move(tracingInformation)),
          functionDefinitions(std::move(functionDefinitions)),
          library(std::move(library)),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});
    }

    void Tracer::initLoadedModules(const ActiveProcessInformation& processInformation)
    {
        auto memoryRegions = processInformation.memoryRegionExtractor->extractAllMemoryRegions();

        for (const auto& memoryRegionDescriptor : *memoryRegions)
        {
            auto filename = library->splitFilenameFromRegionName(memoryRegionDescriptor.moduleName);

            if ((library->isTraceableLibrary(memoryRegionDescriptor.moduleName)) &&
                (!loadedModules.contains(*filename)))
            {
                loadedModules.try_emplace(*filename, memoryRegionDescriptor.base);
            }
        }
    }

    std::map<std::string, uint64_t, std::less<>> Tracer::getLoadedModules() const
    {
        return loadedModules;
    }

    void Tracer::addHooks(const ActiveProcessInformation& processInformation)
    {
        auto processTracingInformation = getProcessTracingInformation(processInformation);

        if (!shouldProcessBeMonitored(processInformation, processTracingInformation))
        {
            return;
        }

        tracedProcesses.try_emplace(processInformation.pid, processInformation.name);
        initLoadedModules(processInformation);
        try
        {
            injectHooks(processInformation, processTracingInformation);
        }
        catch (const std::exception& e)
        {
            tracedProcesses.erase(processInformation.pid);
            logger->warning(
                "Could not trace process",
                {{"Process", processInformation.name}, {"Pid", processInformation.pid}, {"Exception", e.what()}});
        }
    }

    void Tracer::injectHooks(const ActiveProcessInformation& processInformation,
                             const std::optional<ProcessInformation>& processTracingInformation)
    {
        auto introspectionAPI = pluginInterface->getIntrospectionAPI();
        for (const auto& moduleHookTarget : processTracingInformation->modules)
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
                    auto addressWidth = processInformation.is32BitProcess ? ConstantDefinitions::x86AddressWidth
                                                                          : ConstantDefinitions::x64AddressWidth;
                    auto definitions = functionDefinitions->getFunctionParameterDefinitions(
                        moduleHookTarget.name, functionName, addressWidth);
                    auto extractor = std::make_shared<Extractor>(introspectionAPI, addressWidth);
                    auto functionHook = std::make_shared<FunctionHook>(
                        moduleHookTarget.name, functionName, extractor, introspectionAPI, definitions, pluginInterface);
                    functionHook->hookFunction(moduleBaseAddress, processInformation.processCR3);
                    hookList.push_back(functionHook);
                }
                catch (const std::exception& e)
                {
                    logger->warning("Could not trace function",
                                    {{"Library", moduleHookTarget.name},
                                     {"Function", functionName},
                                     {"Process", processInformation.name},
                                     {"Pid", processInformation.pid},
                                     {"Exception", e.what()}});
                }
            }
        }
    }

    void Tracer::removeHooks() const
    {
        for (const auto& hook : hookList)
        {
            hook->teardown();
        }
    }

    std::optional<ProcessInformation>
    Tracer::getProcessTracingInformation(const ActiveProcessInformation& processInformation) const
    {
        for (auto processTracingInformation : *tracingTargetsInformation)
        {
            if (processTracingInformation.name == *processInformation.fullName)
            {
                return processTracingInformation;
            }
        }
        return std::nullopt;
    }

    bool Tracer::shouldProcessBeMonitored(const ActiveProcessInformation& processInformation,
                                          const std::optional<ProcessInformation>& processTracingInformation) const
    {
        if (processTracingInformation)
        {
            return true;
        }
        if (tracedProcesses.contains(processInformation.parentPid))
        {
            return true;
        }

        return false;
    }
}
