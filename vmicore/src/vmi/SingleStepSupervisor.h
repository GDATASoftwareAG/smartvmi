#ifndef VMICORE_SINGLESTEPSUPERVISOR_H
#define VMICORE_SINGLESTEPSUPERVISOR_H

#include "../io/ILogging.h"
#include "LibvmiInterface.h"
#include <functional>
#include <memory>
#include <vector>

namespace VmiCore
{
    class ISingleStepSupervisor
    {
      public:
        virtual ~ISingleStepSupervisor() = default;

        virtual void initializeSingleStepEvents() = 0;

        virtual void teardown() = 0;

        virtual void
        setSingleStepCallback(uint vcpuId, const std::function<void(vmi_event_t*)>& eventCallback, uint64_t data) = 0;

      protected:
        ISingleStepSupervisor() = default;
    };

    class SingleStepSupervisor : public ISingleStepSupervisor
    {
      public:
        explicit SingleStepSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                      std::shared_ptr<ILogging> loggingLib);

        ~SingleStepSupervisor() override;

        void initializeSingleStepEvents() override;

        void teardown() override;

        static event_response_t _defaultSingleStepCallback(vmi_instance_t vmiInstance, vmi_event_t* event);

        void setSingleStepCallback(uint vcpuId,
                                   const std::function<void(vmi_event_t*)>& eventCallback,
                                   uint64_t data) override;

        template <class T>
        static std::function<void(vmi_event_t*)> createSingleStepCallback(std::shared_ptr<T> callbackObjectShared,
                                                                          void (T::*callbackFunction)(vmi_event_t*))
        {
            return [callbackObject = std::weak_ptr<T>(callbackObjectShared),
                    callbackFunction](vmi_event_t* singlestepEvent)
            {
                if (auto targetShared = callbackObject.lock())
                {
                    ((*targetShared).*callbackFunction)(singlestepEvent);
                }
                else
                {
                    throw std::runtime_error("Callback target does not exist anymore.");
                }
            };
        }

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::vector<vmi_event_t> singleStepEvents{};
        std::vector<std::function<void(vmi_event_t*)>> callbacks;

        static std::unique_ptr<ILogger> logger;

        event_response_t singleStepCallback(vmi_event_t* event);
    };
}

#endif // VMICORE_SINGLESTEPSUPERVISOR_H
