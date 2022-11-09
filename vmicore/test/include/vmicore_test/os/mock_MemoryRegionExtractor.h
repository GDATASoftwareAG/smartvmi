#ifndef VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H
#define VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H

#include <gmock/gmock.h>
#include <vmicore/os/IMemoryRegionExtractor.h>

namespace VmiCore
{
    class MockMemoryRegionExtractor : public IMemoryRegionExtractor
    {
      public:
        MOCK_METHOD(std::unique_ptr<std::list<MemoryRegion>>, extractAllMemoryRegions, (), (const override));
    };
}

#endif // VMICORE_MOCK_MEMORYREGIONEXTRACTOR_H
