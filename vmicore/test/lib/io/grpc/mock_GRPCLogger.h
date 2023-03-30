#ifndef VMICORE_MOCK_GRPCLOGGER_H
#define VMICORE_MOCK_GRPCLOGGER_H

#include <gmock/gmock.h>
#include <io/grpc/GRPCLogger.h>

namespace VmiCore
{
    class MockGRPCLogger : public ILogger
    {
      public:
        MOCK_METHOD(void, bind, (const std::initializer_list<CxxLogField>&), (override));

        MOCK_METHOD(void, debug, (std::string_view), (const, override));

        MOCK_METHOD(void,
                    debug,
                    (std::string_view message, const std::initializer_list<CxxLogField>&),
                    (const, override));

        MOCK_METHOD(void, info, (std::string_view message), (const, override));

        MOCK_METHOD(void,
                    info,
                    (std::string_view message, const std::initializer_list<CxxLogField>&),
                    (const, override));

        MOCK_METHOD(void, warning, (std::string_view message), (const, override));

        MOCK_METHOD(void,
                    warning,
                    (std::string_view message, const std::initializer_list<CxxLogField>&),
                    (const, override));

        MOCK_METHOD(void, error, (std::string_view message), (const, override));

        MOCK_METHOD(void,
                    error,
                    (std::string_view message, const std::initializer_list<CxxLogField>&),
                    (const, override));
    };
}

#endif // VMICORE_MOCK_GRPCLOGGER_H
