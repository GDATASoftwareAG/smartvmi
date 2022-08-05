#ifndef VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H
#define VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H

#include "../../../src/os/IMemoryRegionExtractor.h"
#include <gmock/gmock.h>

class MockMemoryRegionExtractor : public IMemoryRegionExtractor
{
  public:
    MOCK_METHOD(std::unique_ptr<std::list<MemoryRegion>>, extractAllMemoryRegions, (), (const override));
};

#endif // VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H
