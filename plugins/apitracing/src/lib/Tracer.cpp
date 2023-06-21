#include "Tracer.h"
#include "Filenames.h"
#include <filesystem>
#include <vector>
#include <vmicore/callback.h>

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    Tracer::Tracer(VmiCore::Plugin::PluginInterface* pluginInterface,
                   std::unique_ptr<ITracingTargetsParser> tracingTargetsParser,
                   std::shared_ptr<IFunctionDefinitions> functionDefinitions,
                   std::shared_ptr<ILibrary> library)
        : pluginInterface(pluginInterface),
          tracingTargetsParser(std::move(tracingTargetsParser)),
          functionDefinitions(std::move(functionDefinitions)),
          library(std::move(library)),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        pluginInterface->registerProcessStartEvent(VMICORE_SETUP_MEMBER_CALLBACK(addHooks));
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
        auto processTracingProfile = getProcessTracingProfile(*processInformation);
        if (!processTracingProfile)
        {
            return;
        }

        tracedProcesses.emplace(processInformation->pid, processTracingProfile.value());
        initLoadedModules(*processInformation);
        try
        {
            injectHooks(*processInformation, processTracingProfile.value());
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
                             const TracingProfile& processTracingProfile)
    {
        auto introspectionAPI = pluginInterface->getIntrospectionAPI();
        for (const auto& moduleHookTarget : processTracingProfile.modules)
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

    std::optional<TracingProfile>
    Tracer::getProcessTracingProfile(const ActiveProcessInformation& processInformation) const
    {
        auto parentProcess = tracedProcesses.find(processInformation.parentPid);
        if (parentProcess == tracedProcesses.end())
        {
            return tracingTargetsParser->getTracingProfile(*processInformation.fullName);
        }

        if (!parentProcess->second.traceChilds)
        {
            return std::nullopt;
        }

        // use the same config as the traced parent
        return parentProcess->second;
    }
}
