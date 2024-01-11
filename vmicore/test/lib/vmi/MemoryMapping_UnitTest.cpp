#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include <gtest/gtest.h>
#include <vmi/MemoryMapping.h>

using testing::NiceMock;

namespace VmiCore
{
    TEST(MemoryMappingTest, getMappedRegions_validState_mappings)
    {
        auto memoryMapping = MemoryMapping(std::make_shared<NiceMock<MockLogging>>(),
                                           std::make_shared<NiceMock<MockLibvmiInterface>>(),
                                           mapped_regions_t{});

        EXPECT_NO_THROW(auto _mappings = memoryMapping.getMappedRegions());
    }

    TEST(MemoryMappingTest, getMappedRegions_alreadyUnmapped_throws)
    {
        auto memoryMapping = MemoryMapping(std::make_shared<NiceMock<MockLogging>>(),
                                           std::make_shared<NiceMock<MockLibvmiInterface>>(),
                                           mapped_regions_t{});

        memoryMapping.unmap();

        EXPECT_ANY_THROW(auto _mappings = memoryMapping.getMappedRegions());
    }
}
