#ifndef VMICORE_IFILETRANSPORT_H
#define VMICORE_IFILETRANSPORT_H

#include <memory>
#include <string_view>
#include <vector>

namespace VmiCore
{
    class IFileTransport
    {
      public:
        virtual ~IFileTransport() = default;

        virtual void saveBinaryToFile(std::string_view logFileName, const std::vector<uint8_t>& data) = 0;

      protected:
        IFileTransport() = default;
    };
}

#endif // VMICORE_IFILETRANSPORT_H
