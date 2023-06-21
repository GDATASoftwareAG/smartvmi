#ifndef APITRACING_MOCK_TRACINGTARGETSPARSER_H
#define APITRACING_MOCK_TRACINGTARGETSPARSER_H

#include "../src/lib/config/TracingTargetsParser.h"
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockTracingTargetsParser : public ITracingTargetsParser
    {
      public:
        MOCK_METHOD(std::optional<TracingProfile>, getTracingProfile, (std::string_view), (const, override));

        MOCK_METHOD(void, addTracingTarget, (const std::string&), (override));
    };
}

#endif // APITRACING_MOCK_TRACINGTARGETSPARSER_H
