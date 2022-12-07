#ifndef APITRACING_MOCK_CONFIG_H
#define APITRACING_MOCK_CONFIG_H

#include "../src/lib/Config.h"
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockConfig : public ApiTracing::IConfig
    {
      public:
        MOCK_METHOD(std::filesystem::path, getTracingTargetsPath, (), (const, override));
    };
}
#endif // APITRACING_MOCK_CONFIG_H
