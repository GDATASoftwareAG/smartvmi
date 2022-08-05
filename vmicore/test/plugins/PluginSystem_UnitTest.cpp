#include "../vmi/ProcessesMemoryState.h"
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::UnorderedElementsAre;
using testing::Unused;

class PluginSystemFixture : public ProcessesMemoryStateFixture
{
  protected:
    Plugin::PluginInterface* pluginInterface{};

    void SetUp() override
    {
        ProcessesMemoryStateFixture::SetUp();
        ProcessesMemoryStateFixture::setupActiveProcesses();
        pluginInterface = dynamic_cast<Plugin::PluginInterface*>(pluginSystem.get());

        setupActiveProcessList({process0, process4, process248});

        process4VadTreeMemoryState();

        activeProcessesSupervisor->initialize();
    }
};

MATCHER_P(IsEqualMemoryRegion, expectedRegion, "")
{
    bool isEqual = true;
    if (expectedRegion.base != arg.base)
    {
        *result_listener << "\nBase Addresses not equal:";
        *result_listener << "\nExpected: " << expectedRegion.base;
        *result_listener << "\nActual: " << arg.base;
        isEqual = false;
    }
    if (expectedRegion.size != arg.size)
    {
        *result_listener << "\nSize not equal:";
        *result_listener << "\nExpected: " << expectedRegion.size;
        *result_listener << "\nActual: " << arg.size;
        isEqual = false;
    }
    if (expectedRegion.moduleName != arg.moduleName)
    {
        *result_listener << "\nModule name not equal: ";
        *result_listener << "\nExpected: " << expectedRegion.moduleName;
        *result_listener << "\nActual: " << arg.moduleName;
        isEqual = false;
    }
    if (expectedRegion.isSharedMemory != arg.isSharedMemory)
    {
        *result_listener << "\nisSharedMemory boolean not equal: ";
        *result_listener << "\nExpected: " << expectedRegion.isSharedMemory;
        *result_listener << "\nActual: " << arg.isSharedMemory;
        isEqual = false;
    }
    if (expectedRegion.isBeingDeleted != arg.isBeingDeleted)
    {
        *result_listener << "\nisBeingDeleted boolean not equal: ";
        *result_listener << "\nExpected: " << expectedRegion.isBeingDeleted;
        *result_listener << "\nActual: " << arg.isBeingDeleted;
        isEqual = false;
    }
    if (expectedRegion.isProcessBaseImage != arg.isProcessBaseImage)
    {
        *result_listener << "\nisProcessBaseImage boolean not equal: ";
        *result_listener << "\nExpected: " << expectedRegion.isProcessBaseImage;
        *result_listener << "\nActual: " << arg.isProcessBaseImage;
        isEqual = false;
    }
    return isEqual;
}

MATCHER_P(IsEqualProcessInformation, expectedProcess, "")
{
    bool isEqual = true;
    if (expectedProcess.processId != arg->pid)
    {
        isEqual = false;
    }
    if ((*arg->fullName) != expectedProcess.fullName)
    {
        isEqual = false;
    }
    return isEqual;
}

TEST_F(PluginSystemFixture, getRunningProcesses_queryMemoryRegionsOfValidProcessWithVadTreeCycle_validMemoryRegions)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });

    EXPECT_THAT(*process4Info->memoryRegionExtractor->extractAllMemoryRegions(),
                UnorderedElementsAre(IsEqualMemoryRegion(expectedMemoryRegion1),
                                     IsEqualMemoryRegion(expectedMemoryRegion2),
                                     IsEqualMemoryRegion(expectedMemoryRegion3)));
}

TEST_F(PluginSystemFixture, getRunningProcesses_validInternalState_CorrectProcesses)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());

    EXPECT_THAT(*processes,
                UnorderedElementsAre(IsEqualProcessInformation(process4), IsEqualProcessInformation(process248)));
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4_region2IsSharedMemory)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    EXPECT_EQ(std::next(memoryRegions->cbegin())->isSharedMemory, true);
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4_region3IsPrivateMemory)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    auto regionIterator = memoryRegions->cbegin();
    std::advance(regionIterator, 2);
    EXPECT_EQ(regionIterator->isSharedMemory, false);
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4_region2IsProcessBaseImage)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    EXPECT_EQ(std::next(memoryRegions->cbegin())->isProcessBaseImage, true);
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4_region3IsNotProcessBaseImage)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    auto regionIterator = memoryRegions->cbegin();
    std::advance(regionIterator, 2);
    EXPECT_EQ(regionIterator->isProcessBaseImage, false);
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4WithHighVPNRegion_hasCorrectBaseAddress)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    auto regionIterator = memoryRegions->cbegin();
    std::advance(regionIterator, 2);
    EXPECT_EQ(regionIterator->base, vadRootNodeLeftChildStartingAddress);
}

TEST_F(PluginSystemFixture, getRunningProcesses_process4WithHighVPNRegion_hasCorrectSize)
{
    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> processes;

    ASSERT_NO_THROW(processes = pluginInterface->getRunningProcesses());
    auto process4Info = *std::find_if(processes->cbegin(),
                                      processes->cend(),
                                      [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                                      { return a->pid == process4.processId; });
    auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

    auto regionIterator = memoryRegions->cbegin();
    std::advance(regionIterator, 2);
    EXPECT_EQ(regionIterator->size, vadRootNodeLeftChildMemoryRegionSize);
}

struct memoryRegionTestInformation
{
    uint64_t virtualAddress;
    uint64_t cr3;
    size_t contentSize;
    std::vector<uint8_t> memoryPageContent;
};

class ReadProcessMemoryRegionFixture : public PluginSystemFixture
{
  protected:
    uint64_t unalignedVA = 1234;
    uint64_t singlePageRegionBaseVA = 1234 * PagingDefinitions::pageSizeInBytes;
    uint64_t threePagesRegionBaseVA = 2345 * PagingDefinitions::pageSizeInBytes;
    uint64_t sevenPagesRegionBaseVA = 6666 * PagingDefinitions::pageSizeInBytes;

    memoryRegionTestInformation singlePageMemoryRegion{singlePageRegionBaseVA,
                                                       systemCR3,
                                                       PagingDefinitions::pageSizeInBytes,
                                                       std::vector<uint8_t>(PagingDefinitions::pageSizeInBytes, 0xCD)};
    std::unique_ptr<std::vector<memoryRegionTestInformation>> threePagesMemoryRegion;
    std::unique_ptr<std::vector<memoryRegionTestInformation>> sevenPagesMemoryRegionInfo;

    std::unique_ptr<std::vector<memoryRegionTestInformation>>
    createMultipageRegionInformation(uint64_t baseVA, uint64_t cr3, size_t numberOfBytes)
    {
        auto resultVector = std::make_unique<std::vector<memoryRegionTestInformation>>();
        if (numberOfBytes > 0)
        {
            uint64_t numberOfSubsequentPages =
                ((baseVA + numberOfBytes - 1) >> PagingDefinitions::numberOfPageIndexBits) -
                (baseVA >> PagingDefinitions::numberOfPageIndexBits);
            for (uint64_t i = 0; i <= numberOfSubsequentPages; ++i)
            {
                uint64_t currentPageContentSize =
                    i == numberOfSubsequentPages
                        ? numberOfBytes - (numberOfSubsequentPages * PagingDefinitions::pageSizeInBytes)
                        : PagingDefinitions::pageSizeInBytes;
                resultVector->push_back({baseVA + (i * PagingDefinitions::pageSizeInBytes),
                                         cr3,
                                         currentPageContentSize,
                                         std::vector<uint8_t>(currentPageContentSize, static_cast<uint8_t>(i))});
            }
        }
        return resultVector;
    }

    void setupThreePagesRegionReturns()
    {
        threePagesMemoryRegion =
            createMultipageRegionInformation(threePagesRegionBaseVA, systemCR3, 3 * PagingDefinitions::pageSizeInBytes);
        threePagesMemoryRegion->at(1).memoryPageContent.clear(); // simulate non mapped page
        for (const auto& element : *threePagesMemoryRegion)
        {
            setupMemoryRegionReturns(element);
        }
    }

    void setupMemoryRegionReturns(const memoryRegionTestInformation& memoryRegionInfo)
    {
        if (!memoryRegionInfo.memoryPageContent.empty())
        {
            ON_CALL(*mockVmiInterface, readXVA(memoryRegionInfo.virtualAddress, memoryRegionInfo.cr3, _))
                .WillByDefault(
                    [memoryPageContent = memoryRegionInfo.memoryPageContent](
                        uint64_t virtualAddress, uint64_t cr3, std::vector<uint8_t>& buffer)
                    {
                        buffer = memoryPageContent;
                        return true;
                    });
        }
        else
        {
            ON_CALL(*mockVmiInterface, readXVA(memoryRegionInfo.virtualAddress, memoryRegionInfo.cr3, _))
                .WillByDefault(Return(false));
        }
    }

    void setupSevenPagesRegionReturns()
    {
        sevenPagesMemoryRegionInfo = createMultipageRegionInformation(6666 * PagingDefinitions::pageSizeInBytes,
                                                                      process4.directoryTableBase,
                                                                      7 * PagingDefinitions::pageSizeInBytes);
        sevenPagesMemoryRegionInfo->at(0).memoryPageContent.clear();
        sevenPagesMemoryRegionInfo->at(1).memoryPageContent.clear();
        sevenPagesMemoryRegionInfo->at(3).memoryPageContent.clear();
        sevenPagesMemoryRegionInfo->at(4).memoryPageContent.clear();
        sevenPagesMemoryRegionInfo->at(6).memoryPageContent.clear();
        for (const auto& element : *sevenPagesMemoryRegionInfo)
        {
            setupMemoryRegionReturns(element);
        }
    }

    void SetUp() override
    {
        PluginSystemFixture::SetUp();

        setupMemoryRegionReturns(singlePageMemoryRegion);
        setupThreePagesRegionReturns();
        setupSevenPagesRegionReturns();
    }
};

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_virtualAddressNotPageAligned_invalidArgumentException)
{
    size_t numberOfBytes = 4;
    std::unique_ptr<std::vector<uint8_t>> data;

    EXPECT_THROW(data = pluginInterface->readProcessMemoryRegion(process4.processId, unalignedVA, numberOfBytes),
                 std::invalid_argument);
}

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_NumberOfBytesIsZero_emptyVector)
{
    size_t numberOfBytes = 0;
    std::unique_ptr<std::vector<uint8_t>> data;

    ASSERT_NO_THROW(
        data = pluginInterface->readProcessMemoryRegion(process4.processId, singlePageRegionBaseVA, numberOfBytes));

    EXPECT_TRUE(data->empty());
}

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_unknownPid_invalidArgumentException)
{
    size_t numberOfBytes = singlePageMemoryRegion.contentSize;

    EXPECT_THROW(auto _unused =
                     pluginInterface->readProcessMemoryRegion(unusedPid, singlePageRegionBaseVA, numberOfBytes),
                 std::invalid_argument);
}

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_smallMemoryRegion_validMemoryRegion)
{
    size_t numberOfBytes = singlePageMemoryRegion.contentSize;
    std::unique_ptr<std::vector<uint8_t>> data;

    ASSERT_NO_THROW(
        data = pluginInterface->readProcessMemoryRegion(process4.processId, singlePageRegionBaseVA, numberOfBytes));

    EXPECT_EQ(singlePageMemoryRegion.memoryPageContent, *data);
}

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_memoryRegionWithNonMappedPage_validMemoryRegion)
{
    size_t numberOfBytes = 3 * PagingDefinitions::pageSizeInBytes;
    std::unique_ptr<std::vector<uint8_t>> data;
    std::vector<uint8_t> resultMemoryRegion;

    // First page
    resultMemoryRegion.insert(resultMemoryRegion.end(),
                              threePagesMemoryRegion->at(0).memoryPageContent.cbegin(),
                              threePagesMemoryRegion->at(0).memoryPageContent.cend());
    // Padding
    resultMemoryRegion.insert(resultMemoryRegion.end(), PagingDefinitions::pageSizeInBytes, 0x0);
    // Third page
    resultMemoryRegion.insert(resultMemoryRegion.end(),
                              threePagesMemoryRegion->at(2).memoryPageContent.cbegin(),
                              threePagesMemoryRegion->at(2).memoryPageContent.cend());

    ASSERT_NO_THROW(
        data = pluginInterface->readProcessMemoryRegion(process4.processId, threePagesRegionBaseVA, numberOfBytes));

    EXPECT_EQ(resultMemoryRegion, *data);
}

TEST_F(ReadProcessMemoryRegionFixture, readProcessMemoryRegion_memoryRegionWithManyUnmappedPages_validMemoryRegion)
{
    size_t sevenPagesSizeInBytes = 7 * PagingDefinitions::pageSizeInBytes;
    std::vector<uint8_t> expectedMemoryRegion;
    expectedMemoryRegion.insert(expectedMemoryRegion.end(), PagingDefinitions::pageSizeInBytes, 0x0);
    expectedMemoryRegion.insert(expectedMemoryRegion.end(),
                                sevenPagesMemoryRegionInfo->at(2).memoryPageContent.cbegin(),
                                sevenPagesMemoryRegionInfo->at(2).memoryPageContent.cend());
    expectedMemoryRegion.insert(expectedMemoryRegion.end(), PagingDefinitions::pageSizeInBytes, 0x0);
    expectedMemoryRegion.insert(expectedMemoryRegion.end(),
                                sevenPagesMemoryRegionInfo->at(5).memoryPageContent.cbegin(),
                                sevenPagesMemoryRegionInfo->at(5).memoryPageContent.cend());
    expectedMemoryRegion.insert(expectedMemoryRegion.end(), PagingDefinitions::pageSizeInBytes, 0x0);
    std::unique_ptr<std::vector<uint8_t>> data;

    ASSERT_NO_THROW(data = pluginInterface->readProcessMemoryRegion(
                        process4.processId, sevenPagesRegionBaseVA, sevenPagesSizeInBytes));

    EXPECT_EQ(expectedMemoryRegion, *data);
}
