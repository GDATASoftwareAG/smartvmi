#ifndef VMICORE_IEVENTSTREAM_H
#define VMICORE_IEVENTSTREAM_H

#include <cxx_rust_part/bridge.h>
#include <memory>
#include <string_view>
#include <vector>

namespace VmiCore
{
    class IEventStream
    {
      public:
        virtual ~IEventStream() = default;

        virtual void sendProcessEvent(::grpc::ProcessState processState,
                                      std::string_view processName,
                                      uint32_t processID,
                                      std::string_view cr3) = 0;

        virtual void sendBSODEvent(int64_t code) = 0;

        virtual void sendReadyEvent() = 0;

        virtual void sendTerminationEvent() = 0;

        virtual void sendErrorEvent(std::string_view message) = 0;

        virtual void sendInMemDetectionEvent(std::string_view message) = 0;

      protected:
        IEventStream() = default;
    };
}

#endif // VMICORE_IEVENTSTREAM_H
