#include "Library.h"
#include <filesystem>

namespace ApiTracing::Windows
{
    bool Library::isTraceableLibrary(std::string_view regionName) const
    {
        return std::filesystem::path(regionName).extension() == ".dll";
    }

    std::unique_ptr<std::string> Library::splitFilenameFromRegionName(const std::string& memoryRegionPath) const
    {
        auto filename = std::make_unique<std::string>(memoryRegionPath);
        if (auto pos = memoryRegionPath.rfind('\\'); pos != std::string::npos)
        {
            filename = std::make_unique<std::string>(memoryRegionPath, pos + 1);
        }
        return filename;
    }
}
