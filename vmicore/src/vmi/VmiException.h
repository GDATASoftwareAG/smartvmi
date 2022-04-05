#ifndef VMICORE_VMIEXCEPTION_H
#define VMICORE_VMIEXCEPTION_H

#include <stdexcept>

class VmiException : public std::runtime_error
{
  public:
    explicit VmiException(const std::string& message) : runtime_error(message) {}
};

#endif // VMICORE_VMIEXCEPTION_H
