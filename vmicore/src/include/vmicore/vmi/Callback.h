#ifndef VMICORE_CALLBACK_H
#define VMICORE_CALLBACK_H

#include <stdexcept>
#include <utility>

/**
 * Setup a safe callback to an arbitrary member function. Will fail if the callback target does not exist anymore.
 * The object creating the callback has to inherit from <code>std::enable_shared_from_this\<\></code> for this to work.
 * It is also important to make sure that the object has been created as a shared pointer.
 *
 * @param callbackFunction Name of the member function without any object namespace prepended
 *
 * @returns Lambda function
 */
#define VMICORE_SETUP_SAFE_MEMBER_CALLBACK(callbackFunction)                                                           \
    [callbackObject = weak_from_this()]<typename... Ts>(Ts&&... ts)                                                    \
    {                                                                                                                  \
        if (auto targetShared = callbackObject.lock())                                                                 \
        {                                                                                                              \
            return targetShared->callbackFunction(std::forward<Ts>(ts)...);                                            \
        }                                                                                                              \
        throw std::runtime_error("Callback target does not exist anymore");                                            \
    }

/**
 * Setup a callback to an arbitrary member function. This is not a safe callback, i.e. it does not check whether the
 * callback target still exists prior to invoking the callback function.
 *
 * @param callbackFunction Name of the member function without any object namespace prepended
 *
 * @returns Lambda function
 */
#define VMICORE_SETUP_MEMBER_CALLBACK(callbackFunction)                                                                \
    [this]<typename... Ts>(Ts&&... ts) { return callbackFunction(std::forward<Ts>(ts)...); }

#endif // VMICORE_CALLBACK_H
