#include "RegisterEventSupervisor.h"
#include "VmiException.h"
#include <bit>
#include <source_location>
#include <vmicore/filename.h>

namespace VmiCore
{
    RegisterEventSupervisor::RegisterEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                     std::shared_ptr<ILogging> logging)
        : vmiInterface(std::move(vmiInterface)), logger(logging->newNamedLogger(FILENAME_STEM))
    {
    }

    void RegisterEventSupervisor::teardown()
    {
        if (contextSwitchEvent)
        {
            vmiInterface->clearEvent(*contextSwitchEvent, false);
        }
    }

    void RegisterEventSupervisor::setContextSwitchCallback(const std::function<void(vmi_event_t*)>& eventCallback)
    {
        if (callback)
        {
            throw VmiException(fmt::format("{}: Registering a second context switch callback is not allowed.",
                                           std::source_location::current().function_name()));
        }
        callback = eventCallback;
        initializeRegisterEvent();
        vmiInterface->registerEvent(*contextSwitchEvent);
    }

    event_response_t RegisterEventSupervisor::_defaultRegisterCallback([[maybe_unused]] vmi_instance_t vmi,
                                                                       vmi_event_t* event)
    {
        return std::bit_cast<RegisterEventSupervisor*>(event->data)->registerCallback(event);
    }

    event_response_t RegisterEventSupervisor::registerCallback(vmi_event_t* event) const
    {
        try
        {
            callback(event);
        }
        catch (const std::exception& e)
        {
            logger->error("Unhandled error in callback", {{"Exception", e.what()}});
        }

        return VMI_EVENT_RESPONSE_NONE;
    }

    void RegisterEventSupervisor::initializeRegisterEvent()
    {
        contextSwitchEvent = std::make_unique<vmi_event_t>();
        SETUP_REG_EVENT(contextSwitchEvent, CR3, VMI_REGACCESS_W, 0, RegisterEventSupervisor::_defaultRegisterCallback);
        contextSwitchEvent->reg_event.onchange = true;
        contextSwitchEvent->reg_event.async = 0;
        contextSwitchEvent->data = this;
    }
}
