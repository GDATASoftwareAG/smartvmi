#ifndef VMICORE_MOCK_LEGACYLOGGING_H
#define VMICORE_MOCK_LEGACYLOGGING_H

#include "../../../src/io/IFileTransport.h"
#include <gmock/gmock.h>

class MockLegacyLogging : public IFileTransport
{
  public:
    MOCK_METHOD(void, saveBinaryToFile, (const std::string& logFileName, const std::vector<uint8_t>& data), (override));
};

#endif // VMICORE_MOCK_LEGACYLOGGING_H
