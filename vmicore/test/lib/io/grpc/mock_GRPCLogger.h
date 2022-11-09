#ifndef VMICORE_MOCK_GRPCLOGGER_H
#define VMICORE_MOCK_GRPCLOGGER_H

#include <gmock/gmock.h>
#include <io/grpc/GRPCLogger.h>

namespace VmiCore
{
    class MockGRPCLogger : public ILogger
    {
      public:
        MOCK_METHOD(void, bind, (const std::initializer_list<::rust::Box<::logging::LogField>>&), (override));

        MOCK_METHOD(void, debug, (const std::string_view&), (const, override));

        MOCK_METHOD(void,
                    debug,
                    (const std::string_view& message, const std::initializer_list<::rust::Box<::logging::LogField>>&),
                    (const, override));

        MOCK_METHOD(void, info, (const std::string_view& message), (const, override));

        MOCK_METHOD(void,
                    info,
                    (const std::string_view& message, const std::initializer_list<::rust::Box<::logging::LogField>>&),
                    (const, override));

        MOCK_METHOD(void, warning, (const std::string_view& message), (const, override));

        MOCK_METHOD(void,
                    warning,
                    (const std::string_view& message, const std::initializer_list<::rust::Box<::logging::LogField>>&),
                    (const, override));

        MOCK_METHOD(void, error, (const std::string_view& message), (const, override));

        MOCK_METHOD(void,
                    error,
                    (const std::string_view& message, const std::initializer_list<::rust::Box<::logging::LogField>>&),
                    (const, override));
    };
}

#endif // VMICORE_MOCK_GRPCLOGGER_H
