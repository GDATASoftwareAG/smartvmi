#ifndef APITRACING_MOCK_TRACEDPROCESSFACTORY_H
#define APITRACING_MOCK_TRACEDPROCESSFACTORY_H

#include "../src/lib/TracedProcessFactory.h"
#include <gmock/gmock.h>

namespace ApiTracing
{
    class MockTracedProcessFactory : public ITracedProcessFactory
    {
      public:
        MOCK_METHOD(std::unique_ptr<ITracedProcess>,
                    createTracedProcess,
                    (const std::shared_ptr<const VmiCore::ActiveProcessInformation>& activeProcessInformation,
                     const TracingProfile& tracingProfile),
                    (const, override));
    };
}

#endif // APITRACING_MOCK_TRACEDPROCESSFACTORY_H
