#ifndef VMICORE_BPRESPONSE_H
#define VMICORE_BPRESPONSE_H

namespace VmiCore
{
    /// An enum containing possible responses for breakpoint event user callbacks.
    enum class BpResponse
    {
        /// Continue with regular workflow.
        Continue,
        /// Remove the breakpoint after handling the current event. Similar to calling <tt>breakpoint.remove()</tt>.
        /// However, <tt>breakpoint.remove()</tt> should not be used in event callbacks.
        Deactivate,
    };
}

#endif // VMICORE_BPRESPONSE_H
