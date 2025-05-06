#ifndef VMICORE_VMIEXCEPTION_H
#define VMICORE_VMIEXCEPTION_H

#include <stdexcept>

namespace VmiCore
{
    class VmiException : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}

#endif // VMICORE_VMIEXCEPTION_H
