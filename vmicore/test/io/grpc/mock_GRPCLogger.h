#ifndef VMICORE_MOCK_GRPCLOGGER_H
#define VMICORE_MOCK_GRPCLOGGER_H

#include "../../../src/io/grpc/GRPCLogger.h"
#include <gmock/gmock.h>

class MockGRPCLogger : public ILogger
{
  public:
    MOCK_METHOD(void, bind, (std::initializer_list<::rust::Box<::logging::LogField>>), (override));

    MOCK_METHOD(void, debug, (std::string message), (const, override));
    MOCK_METHOD(void,
                debug,
                (std::string message, std::initializer_list<::rust::Box<::logging::LogField>>),
                (const, override));

    MOCK_METHOD(void, info, (std::string message), (const, override));
    MOCK_METHOD(void,
                info,
                (std::string message, std::initializer_list<::rust::Box<::logging::LogField>>),
                (const, override));

    MOCK_METHOD(void, warning, (std::string message), (const, override));
    MOCK_METHOD(void,
                warning,
                (std::string message, std::initializer_list<::rust::Box<::logging::LogField>>),
                (const, override));

    MOCK_METHOD(void, error, (std::string message), (const, override));
    MOCK_METHOD(void,
                error,
                (std::string message, std::initializer_list<::rust::Box<::logging::LogField>>),
                (const, override));
};

#endif // VMICORE_MOCK_GRPCLOGGER_H
