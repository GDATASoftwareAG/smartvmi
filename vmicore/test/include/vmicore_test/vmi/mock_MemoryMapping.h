#ifndef VMICORE_MOCK_MEMORYMAPPING_H
#define VMICORE_MOCK_MEMORYMAPPING_H

#include <gmock/gmock.h>
#include <vmicore/vmi/IMemoryMapping.h>

namespace VmiCore
{
    class MockMemoryMapping : public IMemoryMapping
    {
      public:
        MOCK_METHOD(std::weak_ptr<std::vector<MappedRegion>>, getMappedRegions, (), (override));

        MOCK_METHOD(std::size_t, getSizeInGuest, (), (override));

        MOCK_METHOD(void, unmap, (), (override));
    };
}

#endif // VMICORE_MOCK_MEMORYMAPPING_H
