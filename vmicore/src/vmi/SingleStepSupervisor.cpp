#include "SingleStepSupervisor.h"
#include "../GlobalControl.h"
#include "../io/ILogging.h"
#include "../io/grpc/GRPCLogger.h"
#include "VmiException.h"

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
    try
    {
        for (auto& event : singleStepEvents)
        {
            vmiInterface->clearEvent(event, false);
        }
    }
    catch (const VmiException&)
    {
        SingleStepSupervisor::logger->warning("Unable to clear single step event during object destruction");
    }
    logger.reset();
}

void SingleStepSupervisor::initializeSingleStepEvents()
{
    auto numberOfVCPUs = vmiInterface->getNumberOfVCPUs();
    SingleStepSupervisor::logger->debug("initialize callbacks", {logfield::create("vcpus", uint64_t(numberOfVCPUs))});
    singleStepEvents = std::vector<vmi_event_t>(numberOfVCPUs);
    callbacks = std::vector<std::function<void(vmi_event_t*)>>(numberOfVCPUs);
}

event_response_t SingleStepSupervisor::singleStepCallback(vmi_event_t* event)
{
    try
    {
        callbacks[event->vcpu_id](event);
    }
    catch (const std::runtime_error& e)
    {
        throw VmiException(std::string(__func__) +
                           ": Callback target for the current single step event does not exist anymore. VCPU_ID = " +
                           std::to_string(event->vcpu_id));
    }
    callbacks[event->vcpu_id] = nullptr;
    event->callback = nullptr;
    vmiInterface->stopSingleStepForVcpu(event, event->vcpu_id);
    event->ss_event.enable = false;
    return VMI_EVENT_RESPONSE_NONE;
}

void SingleStepSupervisor::setSingleStepCallback(uint vcpuId, const std::function<void(vmi_event_t*)>& eventCallback)
{
    if (callbacks[vcpuId])
    {
        throw VmiException(std::string(__func__) +
                           ": Registering a second callback to the current VCPU is not allowed.");
    }
    callbacks[vcpuId] = eventCallback;
    SETUP_SINGLESTEP_EVENT(&singleStepEvents[vcpuId], 0, _defaultSingleStepCallback, true);
    SET_VCPU_SINGLESTEP(singleStepEvents[vcpuId].ss_event, vcpuId);
    vmiInterface->registerEvent(singleStepEvents[vcpuId]);
}

event_response_t SingleStepSupervisor::_defaultSingleStepCallback(__attribute__((unused)) vmi_instance_t vmiInstance,
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
