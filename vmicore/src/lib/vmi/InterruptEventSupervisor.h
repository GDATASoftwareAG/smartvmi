#ifndef VMICORE_INTERRUPTEVENTSUPERVISOR_H
#define VMICORE_INTERRUPTEVENTSUPERVISOR_H

#include "../GlobalControl.h"
#include "../io/ILogging.h"
#include "../os/IActiveProcessesSupervisor.h"
#include "Breakpoint.h"
#include "Event.h"
#include "InterruptGuard.h"
#include "LibvmiInterface.h"
#include "RegisterEventSupervisor.h"
#include "SingleStepSupervisor.h"
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vmicore/io/ILogger.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    class IInterruptEventSupervisor
    {
      public:
        virtual ~IInterruptEventSupervisor() = default;

        virtual void initialize() = 0;

        virtual void teardown() = 0;

        virtual std::shared_ptr<IBreakpoint>
        createBreakpoint(uint64_t targetVA,
                         uint64_t processDtb,
                         const std::function<BpResponse(IInterruptEvent&)>& callbackFunction,
                         bool global) = 0;

        virtual void deleteBreakpoint(IBreakpoint* breakpoint) = 0;

      protected:
        IInterruptEventSupervisor() = default;
    };

    class InterruptEventSupervisor : public IInterruptEventSupervisor,
                                     public std::enable_shared_from_this<InterruptEventSupervisor>
    {
      public:
        explicit InterruptEventSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                          std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                                          std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor,
                                          std::shared_ptr<IRegisterEventSupervisor> registerSupervisor,
                                          std::shared_ptr<ILogging> loggingLib);

        ~InterruptEventSupervisor() noexcept override;

        void initialize() override;

        void teardown() override;

        std::shared_ptr<IBreakpoint>
        createBreakpoint(uint64_t targetVA,
                         uint64_t processDtb,
                         const std::function<BpResponse(IInterruptEvent&)>& callbackFunction,
                         bool global) override;

        void deleteBreakpoint(IBreakpoint* breakpoint) override;

        static event_response_t _defaultInterruptCallback(vmi_instance_t vmi, vmi_event_t* event);

        event_response_t interruptCallback(addr_t interruptPA,
                                           uint32_t vcpuId,
                                           const std::vector<std::shared_ptr<Breakpoint>>& breakpoints);

        void singleStepCallback(__attribute__((unused)) vmi_event_t* singleStepEvent);

        void contextSwitchCallback(vmi_event_t* registerEvent);

      private:
        struct BpPage
        {
            std::map<addr_t, std::vector<std::shared_ptr<Breakpoint>>> Breakpoints;
            std::shared_ptr<InterruptGuard> PageGuard;
        };

        static constexpr uint8_t DONT_REINJECT_INTERRUPT = 0;
        static constexpr uint8_t REINJECT_INTERRUPT = 1;
        static constexpr uint8_t INT3_BREAKPOINT = 0xCC;

        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor;
        std::shared_ptr<IActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<IRegisterEventSupervisor> registerSupervisor;
        std::shared_ptr<ILogging> loggingLib;
        std::unique_ptr<ILogger> logger;

        std::unordered_map<addr_t, uint8_t> originalValuesByTargetPA;
        std::unordered_map<addr_t, BpPage> breakpointsByGFN{};
        std::unordered_map<addr_t, BPStateResponse> paToBreakpointStatus{};
        std::function<void(vmi_event_t*)> singleStepCallbackFunction;
        std::function<void(vmi_event_t*)> contextSwitchCallbackFunction;
        // Event needs to be allocated separately in order to avoid invalidating references (e.g. in libvmi) when the
        // enclosing object is moved or copied. Therefore, it is wrapped in a unique pointer.
        std::unique_ptr<vmi_event_t> event = std::make_unique<vmi_event_t>();
        Event interruptEvent{event.get()};
        std::mutex lock{};
        std::unique_ptr<vmi_event_t> contextSwitchEvent = std::make_unique<vmi_event_t>();

        std::shared_ptr<InterruptGuard> createPageGuard(uint64_t targetVA, uint64_t processDtb, uint64_t targetGFN);

        void storeOriginalValue(addr_t targetPA);

        void clearInterruptEventHandling();

        static void eraseBreakpointAtAddress(std::vector<std::shared_ptr<Breakpoint>>& breakpointsAtAddress,
                                             const IBreakpoint* breakpoint);

        void enableEvent(addr_t targetPA);

        void disableEvent(addr_t targetPA);

        void removeInterrupt(addr_t targetPA);

        static BPStateResponse getNewBreakpointState(reg_t newDtb,
                                                     const std::vector<std::shared_ptr<Breakpoint>>& breakpoints);
        void updateBreakpointState(BPStateResponse state, uint64_t targetPA) const;
    };
}

#endif // VMICORE_INTERRUPTEVENTSUPERVISOR_H
