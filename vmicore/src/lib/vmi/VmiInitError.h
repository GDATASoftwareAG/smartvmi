#ifndef VMICORE_VMIINITERROR_H
#define VMICORE_VMIINITERROR_H

#include <libvmi/events.h>
#include <vmicore/vmi/VmiException.h>

namespace VmiCore
{
    class VmiInitError : public VmiException
    {
      public:
        explicit VmiInitError(vmi_init_error initError);
    };
}

#endif // VMICORE_VMIINITERROR_H
