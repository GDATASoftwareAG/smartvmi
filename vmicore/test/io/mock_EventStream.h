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
                    (::grpc::ProcessState processState,
                     const std::string& processName,
                     uint32_t processID,
                     const std::string& cr3),
                    (override));
        MOCK_METHOD(void, sendBSODEvent, (int64_t code), (override));
        MOCK_METHOD(void, sendReadyEvent, (), (override));
        MOCK_METHOD(void, sendTerminationEvent, (), (override));
        MOCK_METHOD(void, sendErrorEvent, (const std::string& message), (override));
        MOCK_METHOD(void, sendInMemDetectionEvent, (const std::string& message), (override));
    };
}

#endif // VMICORE_MOCK_EVENTSTREAM_H
