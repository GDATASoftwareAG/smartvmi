#include "VmiInitError.h"

std::string initErrorToString(vmi_init_error initError)
{
    switch (initError)
    {
        case vmi_init_error::VMI_INIT_ERROR_DRIVER:
        {
            return {"VMI_INIT_ERROR_DRIVER"};
        }
        case vmi_init_error::VMI_INIT_ERROR_DRIVER_NOT_DETECTED:
        {
            return {"VMI_INIT_ERROR_DRIVER_NOT_DETECTED"};
        }
        case vmi_init_error::VMI_INIT_ERROR_EVENTS:
        {
            return {"VMI_INIT_ERROR_EVENTS"};
        }
        case vmi_init_error::VMI_INIT_ERROR_NO_CONFIG:
        {
            return {"VMI_INIT_ERROR_NO_CONFIG"};
        }
        case vmi_init_error::VMI_INIT_ERROR_NO_CONFIG_ENTRY:
        {
            return {"VMI_INIT_ERROR_NO_CONFIG_ENTRY"};
        }
        case vmi_init_error::VMI_INIT_ERROR_PAGING:
        {
            return {"VMI_INIT_ERROR_PAGING"};
        }
        case vmi_init_error::VMI_INIT_ERROR_VM_NOT_FOUND:
        {
            return {"VMI_INIT_ERROR_VM_NOT_FOUND"};
        }
        case vmi_init_error::VMI_INIT_ERROR_OS:
        {
            return {"VMI_INIT_ERROR_OS"};
        }
        default:
        {
            return ("Unknown init error.");
        }
    }
}

VmiInitError::VmiInitError(vmi_init_error initError)
    : VmiException("Unable to initialize libvmi. Reason: " + initErrorToString(initError))
{
}
