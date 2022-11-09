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

        void initialize();

        void teardown();

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<ILogger> logger;
        uint64_t targetVA;
        uint64_t targetGFN;
        vmi_event_t guardEvent{};
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
