#ifndef VMICORE_INTERRUPTGUARD_H
#define VMICORE_INTERRUPTGUARD_H

#include "../io/ILogging.h"
#include "LibvmiInterface.h"
#include "SingleStepSupervisor.h"
#include <cstdint>
#include <libvmi/events.h>
#include <memory>
#include <vector>

namespace VmiCore
{
    class InterruptGuard
    {
      public:
        InterruptGuard(std::shared_ptr<ILibvmiInterface> vmiInterface,
                       const std::shared_ptr<ILogging>& logging,
                       uint64_t targetVA,
                       uint64_t targetGFN,
                       uint64_t systemCr3);

        // This object has to be non-copyable and non-movable because we store a self reference in a vmi event that we
        // pass to libvmi. Therefore, we need to avoid invalidating this reference.
        InterruptGuard(const InterruptGuard&) = delete;

        InterruptGuard(const InterruptGuard&&) = delete;

        InterruptGuard& operator=(const InterruptGuard&) = delete;

        InterruptGuard& operator=(const InterruptGuard&&) = delete;

        void initialize();

        void teardown();

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<ILogger> logger;
        uint64_t targetVA;
        uint64_t targetGFN;
        vmi_event_t guardEvent{}; // This is okay because the enclosing object is non-copyable and non-movable
        std::vector<uint8_t> shadowPage;
        uint64_t systemCr3;
        emul_read_t emulateReadData{};
        bool interruptGuardHit = false;

        void enableEvent();

        void disableEvent();

        static event_response_t _guardCallback(vmi_instance_t vmiInstance, vmi_event_t* event);

        event_response_t guardCallback(vmi_event_t* event);
    };
}

#endif // VMICORE_INTERRUPTGUARD_H
