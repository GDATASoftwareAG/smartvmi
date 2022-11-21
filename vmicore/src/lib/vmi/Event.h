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

        uint64_t getRax() override;

        uint64_t getRbx() override;

        uint64_t getRcx() override;

        uint64_t getRdx() override;

        uint64_t getRdi() override;

        uint64_t getRip() override;

        uint64_t getCr3() override;

        uint64_t getR8() override;

        uint64_t getR9() override;

        addr_t getGla() override;

        addr_t getGfn() override;

        addr_t getOffset() override;

      private:
        vmi_event_t* libvmiEvent;
    };
}

#endif // VMICORE_EVENT_H
