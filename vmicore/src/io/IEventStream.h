#ifndef VMICORE_IEVENTSTREAM_H
#define VMICORE_IEVENTSTREAM_H

#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <memory>
#include <vector>

namespace VmiCore
{
    class IEventStream
    {
      public:
        virtual ~IEventStream() = default;

        virtual void sendProcessEvent(::grpc::ProcessState processState,
                                      const std::string& processName,
                                      uint32_t processID,
                                      const std::string& cr3) = 0;
        virtual void sendBSODEvent(int64_t code) = 0;
        virtual void sendReadyEvent() = 0;
        virtual void sendTerminationEvent() = 0;
        virtual void sendErrorEvent(const std::string& message) = 0;
        virtual void sendInMemDetectionEvent(const std::string& message) = 0;

      protected:
        IEventStream() = default;
    };
}

#endif // VMICORE_IEVENTSTREAM_H
