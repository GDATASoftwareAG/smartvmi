#ifndef APITRACING_ILIBRARY_H
#define APITRACING_ILIBRARY_H

#include <memory>
#include <string>

namespace ApiTracing
{
    class ILibrary
    {
      public:
        virtual ~ILibrary() = default;

        [[nodiscard]] virtual bool isTraceableLibrary(std::string_view regionName) const = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string>
        splitFilenameFromRegionName(const std::string& memoryRegionName) const = 0;

      protected:
        ILibrary() = default;
    };
}
#endif // APITRACING_ILIBRARY_H
