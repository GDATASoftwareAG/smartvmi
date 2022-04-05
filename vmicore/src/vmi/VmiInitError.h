#ifndef VMICORE_VMIINITERROR_H
#define VMICORE_VMIINITERROR_H

#include "VmiException.h"
#include <libvmi/events.h>

class VmiInitError : public VmiException
{
  public:
    explicit VmiInitError(vmi_init_error initError);
};

#endif // VMICORE_VMIINITERROR_H
