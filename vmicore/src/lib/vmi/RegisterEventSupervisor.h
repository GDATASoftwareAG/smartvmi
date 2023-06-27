#ifndef VMICORE_REGISTEREVENTSUPERVISOR_H
#define VMICORE_REGISTEREVENTSUPERVISOR_H

#include "../io/ILogging.h"
#include "Event.h"
#include "LibvmiInterface.h"
#include <functional>
#include <memory>

namespace VmiCore
{
    class IRegisterEventSupervisor
    {
      public:
        virtual ~IRegisterEventSupervisor() = default;

        virtual void teardown() = 0;

        virtual void initializeDtbMonitoring() = 0;

        virtual void setContextSwitchCallback(const std::function<void(vmi_event_t*)>& eventCallback) = 0;

      protected:
        IRegisterEventSupervisor() = default;
    };

    class RegisterEventSupervisor : public IRegisterEventSupervisor,
                                    public std::enable_shared_from_this<RegisterEventSupervisor>

    {
      public:
        explicit RegisterEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                         std::shared_ptr<ILogging> logging);

        ~RegisterEventSupervisor() noexcept override = default;

        void teardown() override;

        void initializeDtbMonitoring() override;

        void setContextSwitchCallback(const std::function<void(vmi_event_t*)>& eventCallback) override;

        static event_response_t _defaultRegisterCallback([[maybe_unused]] vmi_instance_t vmi, vmi_event_t* event);

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<vmi_event_t> contextSwitchEvent = std::make_unique<vmi_event_t>();
        std::function<void(vmi_event_t*)> callback{};
        std::unique_ptr<ILogger> logger;

        event_response_t registerCallback(vmi_event_t* event) const;

        void clearDtbEventHandling();
    };
}

#endif // VMICORE_REGISTEREVENTSUPERVISOR_H
