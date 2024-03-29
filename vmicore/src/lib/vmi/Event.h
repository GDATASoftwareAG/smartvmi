#ifndef VMICORE_EVENT_H
#define VMICORE_EVENT_H

#include <cstdint>
#include <libvmi/events.h>
#include <vmicore/vmi/events/IInterruptEvent.h>

namespace VmiCore
{
    class Event : public IInterruptEvent
    {
      public:
        explicit Event(vmi_event_t* libvmiEvent);

        ~Event() override = default;

        [[nodiscard]] uint64_t getRax() const override;

        [[nodiscard]] uint64_t getRbx() const override;

        [[nodiscard]] uint64_t getRcx() const override;

        [[nodiscard]] uint64_t getRdx() const override;

        [[nodiscard]] uint64_t getRdi() const override;

        [[nodiscard]] uint64_t getR8() const override;

        [[nodiscard]] uint64_t getR9() const override;

        [[nodiscard]] uint64_t getRip() const override;

        [[nodiscard]] uint64_t getRsp() const override;

        [[nodiscard]] uint64_t getCr3() const override;

        [[nodiscard]] uint64_t getGs() const override;

        [[nodiscard]] addr_t getGla() const override;

        [[nodiscard]] addr_t getGfn() const override;

        [[nodiscard]] addr_t getOffset() const override;

      private:
        vmi_event_t* libvmiEvent;
    };
}

#endif // VMICORE_EVENT_H
