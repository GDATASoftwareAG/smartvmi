#ifndef VMICORE_INTERRUPTGUARD_H
#define VMICORE_INTERRUPTGUARD_H

#define INT3_BREAKPOINT 0xCC

#include "Event.h"
#include "SingleStepSupervisor.h"

class InterruptGuard final : public Event
{
  public:
    InterruptGuard(std::shared_ptr<ILibvmiInterface> vmiInterface,
                   std::unique_ptr<ILogger> logger,
                   uint64_t targetVA,
                   uint64_t targetPA,
                   uint64_t systemCr3);

    ~InterruptGuard() override = default;

    void initialize() override;

    void teardown() override;

  private:
    uint64_t targetVA;
    uint64_t targetPA;
    vmi_event* guardEvent{};
    std::vector<uint8_t> shadowPage;
    uint64_t systemCr3;
    emul_read_t emulateReadData{};
    bool interruptGuardHit = false;

    void enableEvent() override;

    void disableEvent() override;

    static event_response_t _guardCallback(vmi_instance_t vmiInstance, vmi_event_t* event);

    event_response_t guardCallback(vmi_event_t* event);
};

#endif // VMICORE_INTERRUPTGUARD_H
