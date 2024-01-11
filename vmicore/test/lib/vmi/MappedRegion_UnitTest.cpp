#include <bit>
#include <gtest/gtest.h>
#include <libvmi/libvmi.h>
#include <vmicore/vmi/MappedRegion.h>

using VmiCore::MappedRegion;

TEST(MappedRegionTests, structFieldOrdering)
{
    mapped_region_t libvmiRegion{.start_va = 1, .num_pages = 2, .access_ptr = reinterpret_cast<void*>(3)};
    MappedRegion vmicoreRegion{1, 2, reinterpret_cast<void*>(3)};

    EXPECT_EQ(std::bit_cast<MappedRegion>(libvmiRegion), vmicoreRegion);
}
