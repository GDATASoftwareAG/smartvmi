#include "Tracer.h"
#include "Filenames.h"
#include <filesystem>
#include <vmicore/callback.h>

using VmiCore::ActiveProcessInformation;
using VmiCore::Plugin::PluginInterface;

namespace ApiTracing
{
    Tracer::Tracer(VmiCore::Plugin::PluginInterface* pluginInterface,
                   std::unique_ptr<IConfig> config,
                   std::shared_ptr<ITracedProcessFactory> tracedProcessFactory)
        : pluginInterface(pluginInterface),
          config(std::move(config)),
          logger(this->pluginInterface->newNamedLogger(APITRACING_LOGGER_NAME)),
          tracedProcessFactory(std::move(tracedProcessFactory))
    {
        logger->bind({{VmiCore::WRITE_TO_FILE_TAG, LOG_FILENAME}});

        pluginInterface->registerProcessStartEvent(VMICORE_SETUP_MEMBER_CALLBACK(traceProcess));
        pluginInterface->registerProcessTerminationEvent(VMICORE_SETUP_MEMBER_CALLBACK(removeTracedProcess));
    }

    void Tracer::traceProcess(const std::shared_ptr<const ActiveProcessInformation>& processInformation)
    {
        auto processTracingProfile = getProcessTracingProfile(*processInformation);
        if (!processTracingProfile)
        {
            return;
        }

        try
        {
            auto tracedProcess =
                tracedProcessFactory->createTracedProcess(processInformation, processTracingProfile.value());
            tracedProcesses.insert({processInformation->pid, std::move(tracedProcess)});
            logger->debug("Tracing new process", {{"Name", *processInformation->fullName}});
        }
        catch (const std::exception& e)
        {
            logger->warning(
                "Could not trace process",
                {{"Process", processInformation->name}, {"Pid", processInformation->pid}, {"Exception", e.what()}});
        }
    }

    void Tracer::removeTracedProcess(const std::shared_ptr<const ActiveProcessInformation>& processInformation)
    {
        auto tracedProcess = tracedProcesses.extract(processInformation->pid);

        if (!tracedProcess.empty())
        {
            logger->debug("Removing terminated traced process", {{"Name", *processInformation->fullName}});
            tracedProcess.mapped()->removeHooks();
        }
    }

    void Tracer::teardown() noexcept
    {
        for (const auto& [_pid, tracedProcess] : tracedProcesses)
        {
            tracedProcess->removeHooks();
        }

        tracedProcesses.clear();
    }

    std::optional<TracingProfile>
    Tracer::getProcessTracingProfile(const ActiveProcessInformation& processInformation) const
    {
        auto parentProcess = tracedProcesses.find(processInformation.parentPid);
        if (parentProcess == tracedProcesses.end())
        {
            return config->getTracingProfile(*processInformation.fullName);
        }

        if (!parentProcess->second->traceChildren())
        {
            return std::nullopt;
        }

        // use the same config as the traced parent
        return parentProcess->second->getTracingProfile();
    }
}
