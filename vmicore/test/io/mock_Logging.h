#ifndef VMICORE_MOCK_GRPCLOGGING_H
#define VMICORE_MOCK_GRPCLOGGING_H

#include "../../src/io/ILogging.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <gmock/gmock.h>

namespace VmiCore
{
    class MockLogging : public ILogging
    {
      public:
        MOCK_METHOD(void, start, (), (override));
        MOCK_METHOD(void, stop, (const uint64_t&), (override));
        MOCK_METHOD(std::unique_ptr<ILogger>, newLogger, (), (override));
        MOCK_METHOD(std::unique_ptr<ILogger>, newNamedLogger, (const std::string&), (override));
        MOCK_METHOD(void, setLogLevel, (::logging::Level), (override));
    };
}

#endif // VMICORE_MOCK_GRPCLOGGING_H
