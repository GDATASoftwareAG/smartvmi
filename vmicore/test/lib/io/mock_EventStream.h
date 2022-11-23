#ifndef VMICORE_MOCK_EVENTSTREAM_H
#define VMICORE_MOCK_EVENTSTREAM_H

#include <gmock/gmock.h>
#include <io/IEventStream.h>

namespace VmiCore
{
    class MockEventStream : public IEventStream
    {
      public:
        MOCK_METHOD(void,
                    sendProcessEvent,
                    (::grpc::ProcessState, std::string_view, uint32_t, std::string_view),
                    (override));

        MOCK_METHOD(void, sendBSODEvent, (int64_t), (override));

        MOCK_METHOD(void, sendReadyEvent, (), (override));

        MOCK_METHOD(void, sendTerminationEvent, (), (override));

        MOCK_METHOD(void, sendErrorEvent, (std::string_view), (override));

        MOCK_METHOD(void, sendInMemDetectionEvent, (std::string_view), (override));
    };
}

#endif // VMICORE_MOCK_EVENTSTREAM_H
