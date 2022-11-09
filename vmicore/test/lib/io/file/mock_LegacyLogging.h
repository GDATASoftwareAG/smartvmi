#ifndef VMICORE_MOCK_LEGACYLOGGING_H
#define VMICORE_MOCK_LEGACYLOGGING_H

#include <gmock/gmock.h>
#include <io/IFileTransport.h>

namespace VmiCore
{
    class MockLegacyLogging : public IFileTransport
    {
      public:
        MOCK_METHOD(void,
                    saveBinaryToFile,
                    (const std::string_view& logFileName, const std::vector<uint8_t>& data),
                    (override));
    };
}

#endif // VMICORE_MOCK_LEGACYLOGGING_H
