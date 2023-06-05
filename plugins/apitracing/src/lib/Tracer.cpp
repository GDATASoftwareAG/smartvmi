#include "Tracer.h"
#include "Filenames.h"
#include <filesystem>
#include <vector>

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    Tracer::Tracer(VmiCore::Plugin::PluginInterface* pluginInterface,
                   const ITracingTargetsParser& tracingTargetsParser,
                   std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                   std::shared_ptr<ILibrary> library)
        : pluginInterface(pluginInterface),
          tracingTargetConfigs(tracingTargetsParser.getTracingTargets()),
          functionDefinitions(std::move(functionDefinitions)),
          library(std::move(library)),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        pluginInterface->registerProcessStartEvent([this]<typename T>(T&& a) { addHooks(std::forward<T>(a)); });
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

    void Tracer::addHooks(std::shared_ptr<const ActiveProcessInformation> processInformation)
    {
        auto processTracingConfig = getProcessTracingConfig(*processInformation);

        if (!shouldProcessBeMonitored(*processInformation, processTracingConfig))
        {
            return;
        }

        tracedProcesses.try_emplace(processInformation->pid, processInformation->name);
        initLoadedModules(*processInformation);
        try
        {
            injectHooks(*processInformation, processTracingConfig);
        }
        catch (const std::exception& e)
        {
            tracedProcesses.erase(processInformation->pid);
            logger->warning(
                "Could not trace process",
                {{"Process", processInformation->name}, {"Pid", processInformation->pid}, {"Exception", e.what()}});
        }
    }

    void Tracer::injectHooks(const ActiveProcessInformation& processInformation,
                             const std::optional<ProcessTracingConfig>& processTracingConfig)
    {
        auto introspectionAPI = pluginInterface->getIntrospectionAPI();
        for (const auto& moduleHookTarget : processTracingConfig->profile.modules)
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

    std::optional<ProcessTracingConfig>
    Tracer::getProcessTracingConfig(const ActiveProcessInformation& processInformation) const
    {
        for (const auto& tracingTargetConfig : tracingTargetConfigs)
        {
            if (tracingTargetConfig.name == *processInformation.fullName)
            {
                return tracingTargetConfig;
            }
        }
        return std::nullopt;
    }

    bool Tracer::shouldProcessBeMonitored(const VmiCore::ActiveProcessInformation& processInformation,
                                          const std::optional<ProcessTracingConfig>& processTracingConfig) const
    {
        if (processTracingConfig)
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
