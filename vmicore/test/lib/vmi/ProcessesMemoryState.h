#ifndef VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
#define VMICORE_PROCESSESMEMORYSTATEFIXTURE_H

#include "../config/mock_ConfigInterface.h"
#include "../io/file/mock_LegacyLogging.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include <cstring>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <os/PageProtection.h>
#include <os/windows/ActiveProcessesSupervisor.h>
#include <os/windows/KernelOffsets.h>
#include <os/windows/ProtectionValues.h>
#include <plugins/PluginSystem.h>
#include <vmicore/os/PagingDefinitions.h>
#include <vmicore_test/io/mock_Logger.h>

namespace VmiCore
{
    namespace _SUBSECTION_OFFSETS
    {
        constexpr addr_t ControlArea = 0x0;
    }

    namespace _EX_FAST_REF_OFFSETS
    {
        constexpr addr_t Object = 0x0;
    }

    namespace _RTL_BALANCED_NODE_OFFSETS
    {
        constexpr addr_t Left = 0x0;
        constexpr addr_t Right = 0x8;
    }

    namespace _MMVAD_OFFSETS
    {
        constexpr addr_t BaseAddress = 0x0;
        constexpr addr_t Subsection = 0x72;
    }

    namespace __MMVAD_SHORT_OFFSETS
    {
        constexpr addr_t VadNode = 0x0;
        constexpr addr_t StartingVpn = 0x18;
        constexpr addr_t StartingVpnHigh = 0x20;
        constexpr addr_t EndingVpn = 0x1c;
        constexpr addr_t EndingVpnHigh = 0x21;
        constexpr addr_t Flags = 0x30;
    }

    namespace MMVAD_FLAGS_OFFSETS
    {
        constexpr uint16_t protection = 7;
        constexpr uint16_t privateMemory = 20;
    }

    namespace _CONTROL_AREA_OFFSETS
    {
        constexpr addr_t MMSECTION_FLAGS = 56;
        constexpr addr_t FilePointer = 64;
    }

    namespace _FILE_OBJECT_OFFSETS
    {
        constexpr addr_t FileName = 88;
    }

    namespace _SECTION_OFFSETS
    {
        constexpr addr_t ControlArea = 40;
    }

    namespace SECTION_FLAGS_OFFSETS
    {
        constexpr uint16_t beingDeleted = 0;
        constexpr uint16_t image = 5;
        constexpr uint16_t file = 7;
    }

    namespace _KPROCESS_OFFSETS
    {
        constexpr addr_t DirectoryTableBase = 24;
    }

    namespace _EPROCESS_OFFSETS
    {
        constexpr addr_t ActiveProcessLinks = 752;
        constexpr addr_t UniqueProcessId = 744;
        constexpr addr_t VadRoot = 1552;
        constexpr addr_t SectionObject = 952;
        constexpr addr_t InheritedFromUniqueProcessId = 992;
        constexpr addr_t ExitStatus = 1548;
        constexpr addr_t ImageFilePointer = 1096;
        constexpr addr_t ImageFileName = 1104;
    }

    using processValues = struct processValues_t
    {
        uint64_t eprocessBase;
        uint64_t directoryTableBase;
        pid_t processId;
        uint32_t exitStatus;
        std::string imageFileName;
        std::string fullName;
        uint64_t sectionAddress;
        uint64_t cr3;
        uint64_t controlAreaAddress;
        uint32_t sectionFlags;
        uint64_t filePointerAddress;
        std::string filePath;
    };

    class ProcessesMemoryStateFixture : public testing::Test
    {
      protected:
        uint64_t psActiveProcessHeadVA = 0xfffff80140964160;
        uint64_t systemCR3 = 0x1aa000;
        uint32_t statusPending = 0x103;
        pid_t unusedPid = 0x1337;

        std::string emptyFileName{};
        std::string emptyFullName{};

        const processValues process0 = processValues{0xffffe00170130400,
                                                     0,
                                                     0,
                                                     0x32323232,
                                                     emptyFileName,
                                                     emptyFullName,
                                                     0,
                                                     0,
                                                     0,
                                                     createSectionFlags(true, true, true),
                                                     0,
                                                     ""};
        const processValues process4 = processValues{0xffffe00170250700,
                                                     systemCR3,
                                                     4,
                                                     statusPending,
                                                     "System",
                                                     emptyFullName,
                                                     0,
                                                     systemCR3,
                                                     0,
                                                     createSectionFlags(true, true, true),
                                                     0,
                                                     ""};
        const processValues process248 = processValues{0xffffe00172048800,
                                                       0x62061000,
                                                       248,
                                                       statusPending,
                                                       "MWatcherInterp",
                                                       "MWatcherInterpreter.exe",
                                                       0xffffc0014d174fc0,
                                                       0x62061000,
                                                       0xffffe0017204e9f0,
                                                       createSectionFlags(true, true, true),
                                                       0xffffe0017202a9b0,
                                                       R"(\MWatcherInterpreter\MWatcherInterpreter.exe)"};
        const processValues process332 = processValues{0xffffe001721d6080,
                                                       0x550cc000,
                                                       332,
                                                       statusPending,
                                                       "csrss.exe",
                                                       "csrss.exe",
                                                       0xffffc0014d5d8440,
                                                       0x550cc000,
                                                       0xffffe0017218aa40,
                                                       createSectionFlags(true, true, true),
                                                       0xffffe00172192d10,
                                                       R"(\Windows\System32\csrss.exe)"};

        std::shared_ptr<testing::NiceMock<MockLibvmiInterface>> mockVmiInterface =
            std::make_shared<testing::NiceMock<MockLibvmiInterface>>();
        std::shared_ptr<Windows::KernelAccess> kernelAccess;

        std::shared_ptr<testing::NiceMock<MockLogging>> mockLogging = []()
        {
            std::shared_ptr<testing::NiceMock<MockLogging>> ml = std::make_shared<testing::NiceMock<MockLogging>>();

            ON_CALL(*ml, newNamedLogger(testing::_))
                .WillByDefault([](std::string_view) { return std::make_unique<testing::NiceMock<MockLogger>>(); });

            return ml;
        }();

        std::shared_ptr<testing::NiceMock<MockEventStream>> mockEventStream =
            std::make_shared<testing::NiceMock<MockEventStream>>();

        std::shared_ptr<Windows::ActiveProcessesSupervisor> activeProcessesSupervisor;
        std::shared_ptr<InterruptEventSupervisor> interruptEventSupervisor;

        void setupReturnsForVmiInterface()
        {
            ON_CALL(*mockVmiInterface, convertPidToDtb(Windows::SYSTEM_PID)).WillByDefault(testing::Return(systemCR3));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_KPROCESS", "DirectoryTableBase"))
                .WillByDefault(testing::Return(_KPROCESS_OFFSETS::DirectoryTableBase));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "InheritedFromUniqueProcessId"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::InheritedFromUniqueProcessId));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ImageFileName"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::ImageFileName));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "UniqueProcessId"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::UniqueProcessId));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ActiveProcessLinks"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::ActiveProcessLinks));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ExitStatus"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::ExitStatus));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "SectionObject"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::SectionObject));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "VadRoot"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::VadRoot));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ImageFilePointer"))
                .WillByDefault(testing::Return(_EPROCESS_OFFSETS::ImageFilePointer));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_SECTION", "u1"))
                .WillByDefault(testing::Return(_SECTION_OFFSETS::ControlArea));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_CONTROL_AREA", "u"))
                .WillByDefault(testing::Return(_CONTROL_AREA_OFFSETS::MMSECTION_FLAGS));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_CONTROL_AREA", "FilePointer"))
                .WillByDefault(testing::Return(_CONTROL_AREA_OFFSETS::FilePointer));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_FILE_OBJECT", "FileName"))
                .WillByDefault(testing::Return(_FILE_OBJECT_OFFSETS::FileName));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("__MMVAD_SHORT", "VadNode"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::VadNode));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "StartingVpn"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::StartingVpn));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "StartingVpnHigh"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::StartingVpnHigh));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "EndingVpn"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::EndingVpn));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "EndingVpnHigh"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::EndingVpnHigh));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "u"))
                .WillByDefault(testing::Return(__MMVAD_SHORT_OFFSETS::Flags));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD", "Core"))
                .WillByDefault(testing::Return(_MMVAD_OFFSETS::BaseAddress));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD", "Subsection"))
                .WillByDefault(testing::Return(_MMVAD_OFFSETS::Subsection));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_SUBSECTION", "ControlArea"))
                .WillByDefault(testing::Return(_SUBSECTION_OFFSETS::ControlArea));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_EX_FAST_REF", "Object"))
                .WillByDefault(testing::Return(_EX_FAST_REF_OFFSETS::Object));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_RTL_BALANCED_NODE", "Left"))
                .WillByDefault(testing::Return(_RTL_BALANCED_NODE_OFFSETS::Left));
            ON_CALL(*mockVmiInterface, getKernelStructOffset("_RTL_BALANCED_NODE", "Right"))
                .WillByDefault(testing::Return(_RTL_BALANCED_NODE_OFFSETS::Right));
            ON_CALL(*mockVmiInterface, isInitialized()).WillByDefault(testing::Return(true));
            ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "BeingDeleted"))
                .WillByDefault(testing::Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::beingDeleted, 1)));
            ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "Image"))
                .WillByDefault(testing::Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::image, 6)));
            ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "File"))
                .WillByDefault(testing::Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::file, 8)));
            ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMVAD_FLAGS", "Protection"))
                .WillByDefault(testing::Return(std::make_tuple(0, MMVAD_FLAGS_OFFSETS::protection, 12)));
            ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMVAD_FLAGS", "PrivateMemory"))
                .WillByDefault(testing::Return(std::make_tuple(0, MMVAD_FLAGS_OFFSETS::privateMemory, 21)));
            ON_CALL(*mockVmiInterface, getStructSizeFromJson(Windows::KernelStructOffsets::mmvad_flags::structName))
                .WillByDefault(testing::Return(4));
            ON_CALL(*mockVmiInterface, getStructSizeFromJson(Windows::KernelStructOffsets::mmsection_flags::structName))
                .WillByDefault(testing::Return(4));
        }

        void setupProcessWithLink(const processValues& process, uint64_t link)
        {
            ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::ExitStatus, systemCR3))
                .WillByDefault(testing::Return(process.exitStatus));
            ON_CALL(*mockVmiInterface,
                    read64VA(process.eprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks, systemCR3))
                .WillByDefault(testing::Return(link));
            ON_CALL(*mockVmiInterface,
                    extractStringAtVA(process.eprocessBase + _EPROCESS_OFFSETS::ImageFileName, systemCR3))
                .WillByDefault([process = process](uint64_t, uint64_t)
                               { return std::make_unique<std::string>(process.imageFileName); });
            ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::UniqueProcessId, systemCR3))
                .WillByDefault(testing::Return(process.processId));
            ON_CALL(*mockVmiInterface,
                    read64VA(process.eprocessBase + _KPROCESS_OFFSETS::DirectoryTableBase, systemCR3))
                .WillByDefault(testing::Return(process.directoryTableBase));
        }

        void setupActiveProcessList(const std::vector<processValues>& processes)
        {
            ON_CALL(*mockVmiInterface, translateKernelSymbolToVA("PsActiveProcessHead"))
                .WillByDefault(testing::Return(psActiveProcessHeadVA));
            ON_CALL(*mockVmiInterface, read64VA(psActiveProcessHeadVA, systemCR3))
                .WillByDefault(testing::Return(processes[0].eprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks));

            for (auto process = processes.cbegin(); process != processes.cend()--; process++)
            {
                setupProcessWithLink(*process,
                                     std::next(process)->eprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks);
            }
            setupProcessWithLink(processes.back(), psActiveProcessHeadVA);
        }

        void setupExtractProcessPathReturns(const processValues& process)
        {
            ON_CALL(*mockVmiInterface, read64VA(process.eprocessBase + _EPROCESS_OFFSETS::SectionObject, systemCR3))
                .WillByDefault(testing::Return(process.sectionAddress));
            ON_CALL(*mockVmiInterface, read64VA(process.sectionAddress + _SECTION_OFFSETS::ControlArea, systemCR3))
                .WillByDefault(testing::Return(process.controlAreaAddress));
            ON_CALL(*mockVmiInterface,
                    read64VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
                .WillByDefault([processSectionFlags = process.sectionFlags](uint64_t, uint64_t)
                               { return processSectionFlags; });
            ON_CALL(*mockVmiInterface,
                    read32VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
                .WillByDefault([processSectionFlags = process.sectionFlags](uint64_t, uint64_t)
                               { return processSectionFlags; });
            ON_CALL(*mockVmiInterface,
                    read64VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::FilePointer, systemCR3))
                .WillByDefault(testing::Return(process.filePointerAddress));
            ON_CALL(*mockVmiInterface,
                    extractUnicodeStringAtVA(process.filePointerAddress + _FILE_OBJECT_OFFSETS::FileName, systemCR3))
                .WillByDefault([processFilePath = process.filePath](uint64_t, uint64_t)
                               { return std::make_unique<std::string>(processFilePath); });
        }

        void setupExtractProcessInformationReturns(const processValues& process)
        {
            ON_CALL(*mockVmiInterface,
                    read64VA(process.eprocessBase + _KPROCESS_OFFSETS::DirectoryTableBase, systemCR3))
                .WillByDefault(testing::Return(process.cr3));
            ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::UniqueProcessId, systemCR3))
                .WillByDefault(testing::Return(process.processId));
            ON_CALL(*mockVmiInterface,
                    extractStringAtVA(process.eprocessBase + _EPROCESS_OFFSETS::ImageFileName, systemCR3))
                .WillByDefault([processFileName = process.imageFileName](uint64_t, uint64_t)
                               { return std::make_unique<std::string>(processFileName); });

            setupExtractProcessPathReturns(process);
        }
        void setupReturns(const processValues& process)
        {
            setupExtractProcessInformationReturns(process);
        }

        std::filesystem::path pluginDirectory = "/var/lib/test";
        std::shared_ptr<testing::NiceMock<MockConfigInterface>> mockConfigInterface =
            std::make_shared<testing::NiceMock<MockConfigInterface>>();
        std::shared_ptr<testing::NiceMock<MockLegacyLogging>> mockLegacyLogging =
            std::make_shared<testing::NiceMock<MockLegacyLogging>>();

        std::shared_ptr<PluginSystem> pluginSystem;

        void setupReturnsForConfigInterface()
        {
            ON_CALL(*mockConfigInterface, getPluginDirectory()).WillByDefault(testing::Return(pluginDirectory));
        }

        uint64_t vadRootNodeBase = 666 + PagingDefinitions::kernelspaceLowerBoundary;
        uint64_t vadRootNodeRightChildBase = 777 + PagingDefinitions::kernelspaceLowerBoundary;
        uint64_t vadRootNodeLeftChildBase = 999 + PagingDefinitions::kernelspaceLowerBoundary;

        uint32_t vadRootNodeStartingVpn = 333;
        uint32_t vadRootNodeStartingVpnHigh = 0;
        uint32_t vadRootNodeEndingVpn = 334;
        uint32_t vadRootNodeEndingVpnHigh = 0;
        uint64_t vadRootNodeStartingAddress = vadRootNodeStartingVpn << PagingDefinitions::numberOfPageIndexBits;
        uint64_t vadRootNodeEndingAddress =
            ((vadRootNodeEndingVpn + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
        size_t vadRootNodeMemoryRegionSize = vadRootNodeEndingAddress - vadRootNodeStartingAddress + 1;
        std::unique_ptr<IPageProtection> vadRootNodeMemoryRegionProtection = std::make_unique<PageProtection>(
            static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE), OperatingSystem::WINDOWS);
        MemoryRegion expectedMemoryRegion1{vadRootNodeStartingAddress,
                                           vadRootNodeMemoryRegionSize,
                                           std::string{},
                                           std::move(vadRootNodeMemoryRegionProtection),
                                           false,
                                           false,
                                           false};

        uint32_t vadRootNodeRightChildStartingVpn = 444;
        uint32_t vadRootNodeRightChildEndingVpn = 445;
        uint64_t vadRootNodeRightChildStartingAddress = vadRootNodeRightChildStartingVpn
                                                        << PagingDefinitions::numberOfPageIndexBits;
        uint64_t vadRootNodeChildEndingAddress =
            ((vadRootNodeRightChildEndingVpn + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
        uint64_t vadRootNodeChildMemoryRegionSize =
            vadRootNodeChildEndingAddress - vadRootNodeRightChildStartingAddress + 1;
        std::string fileNameString = std::string(R"(\Windows\IAMSYSTEM.exe)");
        std::unique_ptr<IPageProtection> vadRootNodeChildMemoryRegionProtection = std::make_unique<PageProtection>(
            static_cast<uint32_t>(Windows::ProtectionValues::PAGE_EXECUTE_WRITECOPY), OperatingSystem::WINDOWS);
        MemoryRegion expectedMemoryRegion2{vadRootNodeRightChildStartingAddress,
                                           vadRootNodeChildMemoryRegionSize,
                                           fileNameString,
                                           std::move(vadRootNodeChildMemoryRegionProtection),
                                           true,
                                           true,
                                           true};

        uint64_t vadRootNodeLeftChildStartingVpn = 0x100010666;
        uint64_t vadRootNodeLeftChildEndingVpn = 0x100010667;
        uint64_t vadRootNodeLeftChildStartingAddress = vadRootNodeLeftChildStartingVpn
                                                       << PagingDefinitions::numberOfPageIndexBits;
        uint64_t vadRootNodeLeftChildEndingAddress =
            ((vadRootNodeLeftChildEndingVpn + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
        uint64_t vadRootNodeLeftChildMemoryRegionSize =
            vadRootNodeLeftChildEndingAddress - vadRootNodeLeftChildStartingAddress + 1;
        MemoryRegion expectedMemoryRegion3{
            vadRootNodeLeftChildStartingAddress,
            vadRootNodeLeftChildMemoryRegionSize,
            std::string{},
            std::make_unique<PageProtection>(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE),
                                             OperatingSystem::WINDOWS),
            false,
            false,
            false};

        void systemVadTreeRootNodeMemoryState()
        {
            ON_CALL(*mockVmiInterface, read64VA(process4.eprocessBase + _EPROCESS_OFFSETS::VadRoot, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeBase));
            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Left,
                             systemCR3))
                .WillByDefault(testing::Return(vadRootNodeLeftChildBase));
            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Right,
                             systemCR3))
                .WillByDefault(testing::Return(vadRootNodeRightChildBase));
            ON_CALL(*mockVmiInterface, read8VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeStartingVpnHigh));
            ON_CALL(*mockVmiInterface, read8VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeEndingVpnHigh));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeStartingVpn));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeEndingVpn));
            ON_CALL(*mockVmiInterface, read64VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
                .WillByDefault(testing::Return(
                    createMmvadFlags(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE), true)));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
                .WillByDefault(testing::Return(
                    createMmvadFlags(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE), true)));
        }

        void systemVadTreeRightChildOfRootNodeMemoryState()
        {
            uint64_t subsectionAddress = 0x88800 + PagingDefinitions::kernelspaceLowerBoundary;
            uint64_t controlAreaAddress = 0x99900 + PagingDefinitions::kernelspaceLowerBoundary;
            uint64_t filePointerObjectAddress = 0x2340 + PagingDefinitions::kernelspaceLowerBoundary;

            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeRightChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Left,
                             systemCR3))
                .WillByDefault(testing::Return(0));
            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeRightChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Right,
                             systemCR3))
                .WillByDefault(testing::Return(0));
            ON_CALL(*mockVmiInterface,
                    read8VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
                .WillByDefault(testing::Return(0));
            ON_CALL(*mockVmiInterface,
                    read8VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
                .WillByDefault(testing::Return(0));
            ON_CALL(*mockVmiInterface,
                    read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeRightChildStartingVpn));
            ON_CALL(*mockVmiInterface,
                    read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeRightChildEndingVpn));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
                .WillByDefault(testing::Return(
                    createMmvadFlags(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_EXECUTE_WRITECOPY), false)));
            ON_CALL(*mockVmiInterface, read64VA(vadRootNodeRightChildBase + _MMVAD_OFFSETS::Subsection, systemCR3))
                .WillByDefault(testing::Return(subsectionAddress));
            ON_CALL(*mockVmiInterface, read64VA(subsectionAddress + _SUBSECTION_OFFSETS::ControlArea, systemCR3))
                .WillByDefault(testing::Return(controlAreaAddress));
            ON_CALL(*mockVmiInterface, read64VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
                .WillByDefault(testing::Return(process4.sectionFlags));
            ON_CALL(*mockVmiInterface, read32VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
                .WillByDefault(testing::Return(process4.sectionFlags));
            ON_CALL(*mockVmiInterface,
                    read64VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::FilePointer + _EX_FAST_REF_OFFSETS::Object,
                             systemCR3))
                .WillByDefault(testing::Return(filePointerObjectAddress));
            ON_CALL(*mockVmiInterface,
                    extractUnicodeStringAtVA((filePointerObjectAddress) + _FILE_OBJECT_OFFSETS::FileName, systemCR3))
                .WillByDefault([fileNameString = fileNameString](uint64_t, uint64_t)
                               { return std::make_unique<std::string>(fileNameString); });
            ON_CALL(*mockVmiInterface, read64VA(process4.eprocessBase + _EPROCESS_OFFSETS::ImageFilePointer, systemCR3))
                .WillByDefault(testing::Return(filePointerObjectAddress));
        }

        void systemVadTreeLeftChildOfRootNodeMemoryState()
        {
            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeLeftChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Left,
                             systemCR3))
                .WillByDefault(testing::Return(0));
            ON_CALL(*mockVmiInterface,
                    read64VA(vadRootNodeLeftChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                                 _RTL_BALANCED_NODE_OFFSETS::Right,
                             systemCR3))
                .WillByDefault(testing::Return(vadRootNodeBase));
            ON_CALL(*mockVmiInterface,
                    read8VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
                .WillByDefault([vadLeftChildStartingVpn = vadRootNodeLeftChildStartingVpn](uint64_t, uint64_t)
                               { return vadLeftChildStartingVpn >> 32; });
            ON_CALL(*mockVmiInterface,
                    read8VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
                .WillByDefault([vadLeftChildEndingVpn = vadRootNodeLeftChildEndingVpn](uint64_t, uint64_t)
                               { return vadLeftChildEndingVpn >> 32; });
            ON_CALL(*mockVmiInterface,
                    read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeLeftChildStartingVpn));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
                .WillByDefault(testing::Return(vadRootNodeLeftChildEndingVpn));
            ON_CALL(*mockVmiInterface, read64VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
                .WillByDefault(testing::Return(
                    createMmvadFlags(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE), true)));
            ON_CALL(*mockVmiInterface, read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
                .WillByDefault(testing::Return(
                    createMmvadFlags(static_cast<uint32_t>(Windows::ProtectionValues::PAGE_READWRITE), true)));
        }

        static uint32_t createMmvadFlags(uint32_t protection, bool privateMemory)
        {
            uint32_t flags = protection << MMVAD_FLAGS_OFFSETS::protection | privateMemory
                                                                                 << MMVAD_FLAGS_OFFSETS::privateMemory;
            return flags;
        }

        static uint32_t createSectionFlags(bool image, bool beingDeleted, bool file)
        {
            uint64_t flags = image << SECTION_FLAGS_OFFSETS::image |
                             beingDeleted << SECTION_FLAGS_OFFSETS::beingDeleted | file << SECTION_FLAGS_OFFSETS::file;
            return flags;
        }

        void process4VadTreeMemoryState()
        {
            systemVadTreeRootNodeMemoryState();
            systemVadTreeRightChildOfRootNodeMemoryState();
            systemVadTreeLeftChildOfRootNodeMemoryState();
        }

        void setupActiveProcesses()
        {
            setupActiveProcessList({process0, process4, process248});

            std::vector<processValues> processes = {process248, process332};
            for (const auto& process : processes)
            {
                setupReturns(process);
            }
        }

        void SetUp() override
        {
            setupReturnsForVmiInterface();
            setupReturnsForConfigInterface();

            kernelAccess = std::make_shared<Windows::KernelAccess>(mockVmiInterface);
            activeProcessesSupervisor = std::make_shared<Windows::ActiveProcessesSupervisor>(
                mockVmiInterface, kernelAccess, mockLogging, mockEventStream);
            pluginSystem = std::make_shared<PluginSystem>(mockConfigInterface,
                                                          mockVmiInterface,
                                                          activeProcessesSupervisor,
                                                          interruptEventSupervisor,
                                                          mockLegacyLogging,
                                                          mockLogging,
                                                          std::make_shared<testing::NiceMock<MockEventStream>>());
        };
    };
}

#endif // VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
