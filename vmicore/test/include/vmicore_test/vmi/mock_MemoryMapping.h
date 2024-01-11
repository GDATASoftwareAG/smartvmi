#ifndef VMICORE_MOCK_MEMORYMAPPING_H
#define VMICORE_MOCK_MEMORYMAPPING_H

#include <gmock/gmock.h>
#include <vmicore/vmi/IMemoryMapping.h>

namespace VmiCore
{
    class MockMemoryMapping : public IMemoryMapping
    {
      public:
        MOCK_METHOD(std::span<const MappedRegion>, getMappedRegions, (), (const override));

        MOCK_METHOD(void, unmap, (), (override));
    };
}

#endif // VMICORE_MOCK_MEMORYMAPPING_H
