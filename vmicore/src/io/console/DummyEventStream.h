#ifndef VMICORE_DUMMYEVENTSTREAM_H
#define VMICORE_DUMMYEVENTSTREAM_H

#include "../IEventStream.h"

class DummyEventStream : public IEventStream
{
    inline void sendProcessEvent([[maybe_unused]] grpc::ProcessState processState,
                                 [[maybe_unused]] const std::string& processName,
                                 [[maybe_unused]] uint32_t processID,
                                 [[maybe_unused]] const std::string& cr3) override
    {
    }

    inline void sendBSODEvent([[maybe_unused]] int64_t code) override {}

    inline void sendTerminationEvent() override {}

    inline void sendReadyEvent() override {}

    inline void sendErrorEvent([[maybe_unused]] const std::string& message) override {}

    inline void sendInMemDetectionEvent([[maybe_unused]] const std::string& message) override {}
};

#endif // VMICORE_DUMMYEVENTSTREAM_H
