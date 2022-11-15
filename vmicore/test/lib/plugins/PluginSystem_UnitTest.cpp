#include "../vmi/ProcessesMemoryState.h"
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::Return;
using testing::UnorderedElementsAre;
using testing::Unused;

namespace VmiCore
{
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
        if (expectedRegion->base != arg.base)
        {
            *result_listener << "\nBase Addresses not equal:";
            *result_listener << "\nExpected: " << expectedRegion->base;
            *result_listener << "\nActual: " << arg.base;
            isEqual = false;
        }
        if (expectedRegion->size != arg.size)
        {
            *result_listener << "\nSize not equal:";
            *result_listener << "\nExpected: " << expectedRegion->size;
            *result_listener << "\nActual: " << arg.size;
            isEqual = false;
        }
        if (expectedRegion->moduleName != arg.moduleName)
        {
            *result_listener << "\nModule name not equal: ";
            *result_listener << "\nExpected: " << expectedRegion->moduleName;
            *result_listener << "\nActual: " << arg.moduleName;
            isEqual = false;
        }
        if (expectedRegion->isSharedMemory != arg.isSharedMemory)
        {
            *result_listener << "\nisSharedMemory boolean not equal: ";
            *result_listener << "\nExpected: " << expectedRegion->isSharedMemory;
            *result_listener << "\nActual: " << arg.isSharedMemory;
            isEqual = false;
        }
        if (expectedRegion->isBeingDeleted != arg.isBeingDeleted)
        {
            *result_listener << "\nisBeingDeleted boolean not equal: ";
            *result_listener << "\nExpected: " << expectedRegion->isBeingDeleted;
            *result_listener << "\nActual: " << arg.isBeingDeleted;
            isEqual = false;
        }
        if (expectedRegion->isProcessBaseImage != arg.isProcessBaseImage)
        {
            *result_listener << "\nisProcessBaseImage boolean not equal: ";
            *result_listener << "\nExpected: " << expectedRegion->isProcessBaseImage;
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
                          processes->cend(),
                          [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                          { return a->pid == process4.processId; });

        EXPECT_THAT(*process4Info->memoryRegionExtractor->extractAllMemoryRegions(),
                    UnorderedElementsAre(IsEqualMemoryRegion(&expectedMemoryRegion1),
                                         IsEqualMemoryRegion(&expectedMemoryRegion2),
                                         IsEqualMemoryRegion(&expectedMemoryRegion3)));
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
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
        auto process4Info =
            *std::find_if(processes->cbegin(),
                          processes->cend(),
                          [process4 = process4](const std::shared_ptr<const ActiveProcessInformation>& a)
                          { return a->pid == process4.processId; });
        auto memoryRegions = process4Info->memoryRegionExtractor->extractAllMemoryRegions();

        auto regionIterator = memoryRegions->cbegin();
        std::advance(regionIterator, 2);
        EXPECT_EQ(regionIterator->size, vadRootNodeLeftChildMemoryRegionSize);
    }
}
