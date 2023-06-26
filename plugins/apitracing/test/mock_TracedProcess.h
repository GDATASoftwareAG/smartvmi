#ifndef APITRACING_MOCK_TRACEDPROCESS_H
#define APITRACING_MOCK_TRACEDPROCESS_H

#include "TracedProcess.h"
#include <gmock/gmock.h>
#include <memory>

namespace ApiTracing
{
    class MockTracedProcess : public ITracedProcess
    {
      public:
        MOCK_METHOD(void, removeHooks, (), (noexcept, override));

        MOCK_METHOD(bool, traceChildren, (), (const, override));

        MOCK_METHOD(TracingProfile, getTracingProfile, (), (override));
    };
}

#endif // APITRACING_MOCK_TRACEDPROCESS_H
