#ifndef VMICORE_FILENAME_H
#define VMICORE_FILENAME_H

// Source location still prefixed with experimental when using Clang
#include <source_location>
#include <string_view>

// Unix paths only
constexpr const char* stripPath(const char* path)
{
    const auto* lastname = path;
    for (const auto* p = path; *p; ++p)
    {
        if (*p == '/' && *(p + 1))
        {
            lastname = p + 1;
        }
    }
    return lastname;
}

constexpr size_t lastDotAt(const char* string)
{
    auto last_dot_index = 0;
    int i = 0;
    for (; *string; ++string, ++i)
    {
        if (*string == '.')
        {
            last_dot_index = i;
        }
    }
    return last_dot_index ? last_dot_index : i;
}

constexpr ::std::string_view filenameStem(const ::std::source_location& sourceLocation)
{
    auto* fileName = stripPath(sourceLocation.file_name());
    return {fileName, lastDotAt(fileName)};
}

#define FILENAME_STEM filenameStem(::std::source_location::current())

#endif // VMICORE_FILENAME_H
