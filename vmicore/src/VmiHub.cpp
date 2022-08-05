#include "VmiHub.h"
#include "GlobalControl.h"

#include <csignal>
#include <memory>
#include <utility>

namespace
{
    int exitCode = 0;
    const std::string loggerName = std::filesystem::path(__FILE__).filename().stem();
}

VmiHub::VmiHub(std::shared_ptr<IConfigParser> configInterface,
               std::shared_ptr<ILibvmiInterface> vmiInterface,
               std::shared_ptr<PluginSystem> pluginSystem,
               std::shared_ptr<SystemEventSupervisor> systemEventSupervisor,
               std::shared_ptr<ILogging> loggingLib,
               std::shared_ptr<IEventStream> eventStream)
    : configInterface(std::move(configInterface)),
      vmiInterface(std::move(vmiInterface)),
      pluginSystem(std::move(pluginSystem)),
      systemEventSupervisor(std::move(systemEventSupervisor)),
      loggingLib(std::move(loggingLib)),
      logger(NEW_LOGGER(this->loggingLib)),
      eventStream(std::move(eventStream))
{
}

void VmiHub::waitForEvents() const
{
    // TODO: only set postRunPluginAction to true after sample process is started
    GlobalControl::postRunPluginAction = true;
#ifdef TRACE_MODE
    auto loopStart = std::chrono::steady_clock::now();
#endif
    while (!GlobalControl::endVmi)
    {
        try
        {
#ifdef TRACE_MODE
            auto callStart = std::chrono::steady_clock::now();
            vmiInterface->waitForEvent();
            auto currentTime = std::chrono::steady_clock::now();
            auto callDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - callStart);
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - loopStart);
            logger->debug("Event loop call",
                          {logfield::create("durationMilliseconds", callDuration.count()),
                           logfield::create("totalElapsedTimeSeconds", elapsedTime.count())});
#else
            vmiInterface->waitForEvent();
#endif
        }
        catch (const std::exception& e)
        {
            logger->error("Error while waiting for events", {logfield::create("exception", e.what())});
            eventStream->sendErrorEvent(e.what());
            logger->info("Trying to get the VM state");

            if (vmiInterface->isVmAlive())
            {
                logger->info("VM is alive");
            }
            else
            {
                logger->info("VM is not alive");
                GlobalControl::postRunPluginAction = false;
            }
            exitCode = 1;
            GlobalControl::endVmi = true;
        }
        if (!vmiInterface->areEventsPending() && !GlobalControl::endVmi)
        {
            if (!vmiInterface->isVmAlive())
            {
                logger->error("VM is unresponsive");
                eventStream->sendErrorEvent("VM is unresponsive");
                exitCode = 1;
                GlobalControl::endVmi = true;
                GlobalControl::postRunPluginAction = false;
            }
        }
    }
}

void VmiHub::performShutdownPluginAction() const
{
    bool success = true;
    try
    {
        vmiInterface->waitForCR3Event([&pluginSystem = pluginSystem]()
                                      { pluginSystem->passShutdownEventToRegisteredPlugins(); });
    }
    catch (const VmiException& e)
    {
        success = false;
        logger->warning("VmiException", {logfield::create("exception", e.what())});
    }
    if (!success)
    {
        logger->info("Trying alternative approach");
        vmiInterface->pauseVm();
        pluginSystem->passShutdownEventToRegisteredPlugins();
        vmiInterface->resumeVm();
    }
}

void logReceivedSignal(int signal)
{
    if (signal > 0)
    {
        switch (signal)
        {
            case SIGINT:
            {
                GlobalControl::logger()->info("externalInterruptHandler: SIGINT received",
                                              {logfield::create("logger", loggerName)});
                break;
            }
            case SIGTERM:
            {
                GlobalControl::logger()->info("externalInterruptHandler: SIGTERM received",
                                              {logfield::create("logger", loggerName)});
                break;
            }
            default:
            {
                GlobalControl::logger()->error("Called for unhandled signal. This should never occur",
                                               {logfield::create("logger", loggerName)});
            }
        }
    }
}

void externalInterruptHandler(int signal)
{
    exitCode = 128 + signal;
    logReceivedSignal(signal);
    GlobalControl::endVmi = true;
}

void setupSignalHandling()
{
    struct sigaction sigactionStruct
    {
    };
    sigactionStruct.sa_handler = &externalInterruptHandler;
    auto status = sigaction(SIGINT, &sigactionStruct, nullptr);
    if (status != 0)
    {
        throw std::runtime_error("Unable to register SIGINT action handler.");
    }
    status = sigaction(SIGTERM, &sigactionStruct, nullptr);
    if (status != 0)
    {
        throw std::runtime_error("Unable to register SIGTERM action handler.");
    }
}

uint VmiHub::run(const std::unordered_map<std::string, std::vector<std::string>>& pluginArgs)
{
    for (auto& plugin : configInterface->getPlugins())
    {
        pluginSystem->initializePlugin(plugin.first,
                                       plugin.second,
                                       pluginArgs.contains(plugin.first) ? pluginArgs.at(plugin.first)
                                                                         : std::vector<std::string>{plugin.first});
    }

    // Initialize SystemEventSupervisor as first callback for CR3Event to avoid initialization on an undefined memory
    // state
    vmiInterface->initializeVmi([&systemEventSupervisor = systemEventSupervisor]()
                                { systemEventSupervisor->initialize(); });

    eventStream->sendReadyEvent();

    setupSignalHandling();
    waitForEvents();
    if (GlobalControl::postRunPluginAction)
    {
        performShutdownPluginAction();
    }
    systemEventSupervisor->teardown();
    return exitCode;
}
