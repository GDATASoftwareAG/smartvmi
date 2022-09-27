#include "SingleStepSupervisor.h"
#include "../GlobalControl.h"
#include "../io/grpc/GRPCLogger.h"
#include "VmiException.h"
#include <cmath>

namespace VmiCore
{
    namespace
    {
        SingleStepSupervisor* singleStepSupervisorSingleton = nullptr;
    }

    std::unique_ptr<ILogger> SingleStepSupervisor::logger;

    SingleStepSupervisor::SingleStepSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                               std::shared_ptr<ILogging> loggingLib)
        : vmiInterface(std::move(vmiInterface))
    {
        if (singleStepSupervisorSingleton != nullptr)
        {
            throw std::runtime_error("Not allowed to initialize more than one instance of SingleStepSupervisor.");
        }
        singleStepSupervisorSingleton = this;
        SingleStepSupervisor::logger = NEW_LOGGER(loggingLib);
    }

    SingleStepSupervisor::~SingleStepSupervisor()
    {
        singleStepSupervisorSingleton = nullptr;
        logger.reset();
    }

    void SingleStepSupervisor::initializeSingleStepEvents()
    {
        auto numberOfVCPUs = vmiInterface->getNumberOfVCPUs();
        SingleStepSupervisor::logger->debug("initialize callbacks",
                                            {logfield::create("vcpus", static_cast<uint64_t>(numberOfVCPUs))});
        singleStepEvents = std::vector<vmi_event_t>(numberOfVCPUs);
        callbacks = std::vector<std::function<void(vmi_event_t*)>>(numberOfVCPUs);
    }

    void SingleStepSupervisor::teardown()
    {
        for (auto& event : singleStepEvents)
        {
            if (event.ss_event.enable)
            {
                auto vcpu_index = static_cast<uint32_t>(log2(event.ss_event.vcpus));
                callbacks[vcpu_index] = nullptr;
                event.callback = nullptr;
                try
                {
                    vmiInterface->stopSingleStepForVcpu(&event, vcpu_index);
                }
                catch (const VmiException& e)
                {
                    SingleStepSupervisor::logger->error("Unable to clear single step event during teardown",
                                                        {logfield::create("exception", e.what())});
                }
                event.ss_event.enable = false;
            }
        }
    }

    event_response_t SingleStepSupervisor::singleStepCallback(vmi_event_t* event)
    {
        try
        {
            callbacks[event->vcpu_id](event);
        }
        catch (const std::runtime_error& e)
        {
            throw VmiException(fmt::format(
                "{}: Callback target for the current single step event does not exist anymore. VCPU_ID = {}",
                __func__,
                event->vcpu_id));
        }
        callbacks[event->vcpu_id] = nullptr;
        event->callback = nullptr;
        vmiInterface->stopSingleStepForVcpu(event, event->vcpu_id);
        event->ss_event.enable = false;
        return VMI_EVENT_RESPONSE_NONE;
    }

    void SingleStepSupervisor::setSingleStepCallback(uint vcpuId,
                                                     const std::function<void(vmi_event_t*)>& eventCallback)
    {
        if (callbacks[vcpuId])
        {
            throw VmiException(
                fmt::format("{}: Registering a second callback to the current VCPU is not allowed.", __func__));
        }
        callbacks[vcpuId] = eventCallback;
        SETUP_SINGLESTEP_EVENT(&singleStepEvents[vcpuId], 0, _defaultSingleStepCallback, true);
        SET_VCPU_SINGLESTEP(singleStepEvents[vcpuId].ss_event, vcpuId);
        vmiInterface->registerEvent(singleStepEvents[vcpuId]);
    }

    event_response_t SingleStepSupervisor::_defaultSingleStepCallback(__attribute__((unused))
                                                                      vmi_instance_t vmiInstance,
                                                                      vmi_event_t* event)
    {
        auto eventResponse = VMI_EVENT_RESPONSE_NONE;
        try
        {
            eventResponse = singleStepSupervisorSingleton->singleStepCallback(event);
        }
        catch (const std::exception& e)
        {
            GlobalControl::endVmi = true;
            SingleStepSupervisor::logger->error("Unexpected exception", {logfield::create("exception", e.what())});
        }
        return eventResponse;
    }
}
