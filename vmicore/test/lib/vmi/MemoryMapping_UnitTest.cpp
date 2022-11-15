#include "../io/mock_Logging.h"
#include <gtest/gtest.h>
#include <vmi/MemoryMapping.h>
#include <vmicore/os/PagingDefinitions.h>

using testing::NiceMock;
using VmiCore::PagingDefinitions::numberOfPageIndexBits;
using VmiCore::PagingDefinitions::pageSizeInBytes;

namespace VmiCore
{
    constexpr uint64_t testBaseVA = 0x123 << numberOfPageIndexBits;

    TEST(MemoryMappingTest, constructor_emptyAccessPointers_emptyMappings)
    {
        auto accessPointers = std::vector<void*>{};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), 0);
        EXPECT_TRUE(memoryMapping.getMappedRegions().lock()->empty());
    }

    TEST(MemoryMappingTest, constructor_AccessPointersWithNullPointersOnly_emptyMappings)
    {
        auto accessPointers = std::vector<void*>{nullptr, nullptr};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), 2 * pageSizeInBytes);
        EXPECT_TRUE(memoryMapping.getMappedRegions().lock()->empty());
    }

    TEST(MemoryMappingTest, constructor_continguousRegion_correctMapping)
    {
        auto accessPointers = std::vector<void*>{reinterpret_cast<void*>(0x1 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x2 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x3 << numberOfPageIndexBits)};
        auto expectedMappedRegions = std::vector<MappedRegion>{MappedRegion{
            testBaseVA, std::span(reinterpret_cast<uint8_t*>(0x1 << numberOfPageIndexBits), 3 * pageSizeInBytes)}};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), accessPointers.size() * pageSizeInBytes);
        EXPECT_EQ(*memoryMapping.getMappedRegions().lock(), expectedMappedRegions);
    }

    TEST(MemoryMappingTest, constructor_twoRegions_correctMapping)
    {
        auto accessPointers = std::vector<void*>{reinterpret_cast<void*>(0x1 << numberOfPageIndexBits),
                                                 nullptr,
                                                 reinterpret_cast<void*>(0x2 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x3 << numberOfPageIndexBits)};
        auto expectedMappedRegions = std::vector<MappedRegion>{
            {testBaseVA, {reinterpret_cast<uint8_t*>(0x1 << numberOfPageIndexBits), pageSizeInBytes}},
            {testBaseVA + 2 * pageSizeInBytes,
             {reinterpret_cast<uint8_t*>(0x2 << numberOfPageIndexBits), 2 * pageSizeInBytes}}};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), accessPointers.size() * pageSizeInBytes);
        EXPECT_EQ(*memoryMapping.getMappedRegions().lock(), expectedMappedRegions);
    }

    TEST(MemoryMappingTest, constructor_twoRegionsWithlastPageUnmapped_correctMapping)
    {
        auto accessPointers = std::vector<void*>{reinterpret_cast<void*>(0x1 << numberOfPageIndexBits),
                                                 nullptr,
                                                 nullptr,
                                                 reinterpret_cast<void*>(0x2 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x3 << numberOfPageIndexBits),
                                                 nullptr};
        auto expectedMappedRegions = std::vector<MappedRegion>{
            {testBaseVA, {reinterpret_cast<uint8_t*>(0x1 << numberOfPageIndexBits), pageSizeInBytes}},
            {testBaseVA + 3 * pageSizeInBytes,
             {reinterpret_cast<uint8_t*>(0x2 << numberOfPageIndexBits), 2 * pageSizeInBytes}}};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), accessPointers.size() * pageSizeInBytes);
        EXPECT_EQ(*memoryMapping.getMappedRegions().lock(), expectedMappedRegions);
    }

    TEST(MemoryMappingTest, constructor_threeRegions_correctMapping)
    {
        auto accessPointers = std::vector<void*>{reinterpret_cast<void*>(0x1 << numberOfPageIndexBits),
                                                 nullptr,
                                                 nullptr,
                                                 reinterpret_cast<void*>(0x2 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x3 << numberOfPageIndexBits),
                                                 nullptr,
                                                 reinterpret_cast<void*>(0x4 << numberOfPageIndexBits),
                                                 nullptr};
        auto expectedMappedRegions = std::vector<MappedRegion>{
            {testBaseVA, {reinterpret_cast<uint8_t*>(0x1 << numberOfPageIndexBits), pageSizeInBytes}},
            {testBaseVA + 3 * pageSizeInBytes,
             {reinterpret_cast<uint8_t*>(0x2 << numberOfPageIndexBits), 2 * pageSizeInBytes}},
            {testBaseVA + 6 * pageSizeInBytes,
             {reinterpret_cast<uint8_t*>(0x4 << numberOfPageIndexBits), pageSizeInBytes}}};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), accessPointers.size() * pageSizeInBytes);
        EXPECT_EQ(*memoryMapping.getMappedRegions().lock(), expectedMappedRegions);
    }

    TEST(MemoryMappingTest, constructor_firstTwoPagesUnmapped_correctMapping)
    {
        auto accessPointers = std::vector<void*>{nullptr,
                                                 nullptr,
                                                 reinterpret_cast<void*>(0x1 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x2 << numberOfPageIndexBits),
                                                 reinterpret_cast<void*>(0x3 << numberOfPageIndexBits)};
        auto expectedMappedRegions = std::vector<MappedRegion>{
            {testBaseVA + 2 * pageSizeInBytes,
             {reinterpret_cast<uint8_t*>(0x1 << numberOfPageIndexBits), 3 * pageSizeInBytes}}};

        auto memoryMapping = MemoryMapping(testBaseVA, accessPointers, std::make_shared<NiceMock<MockLogging>>());

        EXPECT_EQ(memoryMapping.getSizeInGuest(), accessPointers.size() * pageSizeInBytes);
        EXPECT_EQ(*memoryMapping.getMappedRegions().lock(), expectedMappedRegions);
    }
}
