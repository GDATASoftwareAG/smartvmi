#ifndef VMICORE_MOCK_EVENTSTREAM_H
#define VMICORE_MOCK_EVENTSTREAM_H

#include "../../src/io/IEventStream.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <gmock/gmock.h>

namespace VmiCore
{
    class MockEventStream : public IEventStream
    {
      public:
        MOCK_METHOD(void,
                    sendProcessEvent,
                    (::grpc::ProcessState, const std::string_view&, uint32_t, const std::string_view&),
                    (override));

        MOCK_METHOD(void, sendBSODEvent, (int64_t), (override));

        MOCK_METHOD(void, sendReadyEvent, (), (override));

        MOCK_METHOD(void, sendTerminationEvent, (), (override));

        MOCK_METHOD(void, sendErrorEvent, (const std::string_view&), (override));

        MOCK_METHOD(void, sendInMemDetectionEvent, (const std::string_view&), (override));
    };
}

#endif // VMICORE_MOCK_EVENTSTREAM_H
