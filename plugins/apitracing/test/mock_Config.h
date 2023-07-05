#ifndef APITRACING_MOCK_CONFIG_H
#define APITRACING_MOCK_CONFIG_H

#include <config/Config.h>
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockConfig : public IConfig
    {
      public:
        MOCK_METHOD(std::optional<TracingProfile>, getTracingProfile, (std::string_view), (const, override));

        MOCK_METHOD(std::filesystem::path, getFunctionDefinitionsPath, (), (const, override));

        MOCK_METHOD(void, addTracingTarget, (const std::string&), (override));

        MOCK_METHOD(void, setFunctionDefinitionsPath, (const std::filesystem::path&), (override));
    };
}

#endif // APITRACING_MOCK_CONFIG_H
