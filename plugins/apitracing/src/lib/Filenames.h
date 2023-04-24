#ifndef APITRACING_FILENAMES_H
#define APITRACING_FILENAMES_H

#include <vmicore/filename.h>

#define APITRACING_LOGGER_NAME std::string("ApiTracing_").append(FILENAME_STEM)

namespace ApiTracing
{
    constexpr const char* LOG_FILENAME = "apiTracing.txt";
}
#endif // APITRACING_FILENAMES_H
