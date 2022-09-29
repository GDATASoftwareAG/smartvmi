#ifndef VMICORE_INTERRUPTEVENT_H
#define VMICORE_INTERRUPTEVENT_H

#include "../io/ILogging.h"
#include "Event.h"
#include "InterruptGuard.h"
#include "SingleStepSupervisor.h"
#include <config.h>
#include <map>

#define DONT_REINJECT_INTERRUPT 0
#define REINJECT_INTERRUPT 1
#define INT3_BREAKPOINT 0xCC
#define BRK64_BREAKPOINT 0xD4200000

class InterruptEvent final : public Event, public std::enable_shared_from_this<InterruptEvent>
{
  public:
    enum class InterruptResponse
    {
        Continue,
        Deactivate,
    };

    InterruptEvent(std::shared_ptr<ILibvmiInterface> vmiInterface,
                   uint64_t targetPA,
                   std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                   std::unique_ptr<InterruptGuard> interruptGuard,
                   std::function<InterruptResponse(InterruptEvent&)> callbackFunction,
                   std::unique_ptr<ILogger> logger);

    ~InterruptEvent() override;

    void initialize() override;

    void teardown() override;

    static registers_t* getRegisters();

    static void initializeInterruptEventHandling(ILibvmiInterface& vmiInterface);

    static void clearInterruptEventHandling(ILibvmiInterface& vmiInterface);

    static event_response_t _defaultInterruptCallback(vmi_instance_t vmi, vmi_event_t* event);

    template <class T>
    static std::function<InterruptResponse(InterruptEvent&)>
    createInterruptCallback(std::weak_ptr<T> callbackObject, InterruptResponse (T::*callbackFunction)(InterruptEvent&))
    {
        return [callbackObject, callbackFunction](InterruptEvent& interruptEvent)
        {
            if (auto targetShared = callbackObject.lock())
            {
                return ((*targetShared).*callbackFunction)(interruptEvent);
            }
            else
            {
                throw std::runtime_error("Callback target does not exist anymore.");
            }
        };
    }

  private:
    uint64_t targetPA;
    std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor;
    std::unique_ptr<InterruptGuard> interruptGuard;
    std::function<InterruptResponse(InterruptEvent&)> callbackFunction;
    std::function<void(vmi_event_t*)> singleStepCallbackFunction;
    std::string targetPAString;

#if defined(X86_64)
    uint8_t originalValue = 0;
#elif defined(ARM64)
    uint32_t originalValue = 0;
#endif

    void enableEvent() override;

    void disableEvent() override;

    void setupVmiInterruptEvent();

    void storeOriginalValue();

    event_response_t interruptCallback(uint32_t vcpuId);

    void singleStepCallback(vmi_event_t* singleStepEvent);
};

#endif // VMICORE_INTERRUPTEVENT_H
