#ifndef VMICORE_VMIINITERROR_H
#define VMICORE_VMIINITERROR_H

#include <vmicore/vmi/VmiException.h>
#include <libvmi/events.h>

namespace VmiCore
{
    class VmiInitError : public VmiException
    {
      public:
        explicit VmiInitError(vmi_init_error initError);
    };
}

#endif // VMICORE_VMIINITERROR_H
