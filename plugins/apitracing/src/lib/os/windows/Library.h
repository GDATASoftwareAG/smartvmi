#ifndef APITRACING_LIBRARY_H
#define APITRACING_LIBRARY_H

#include "../ILibrary.h"
#include <memory>

namespace ApiTracing::Windows
{
    class Library : public ILibrary
    {
      public:
        [[nodiscard]] bool isTraceableLibrary(std::string_view regionName) const override;

        [[nodiscard]] std::unique_ptr<std::string>
        splitFilenameFromRegionName(const std::string& memoryRegionPath) const override;
    };
}
#endif // APITRACING_LIBRARY_H
