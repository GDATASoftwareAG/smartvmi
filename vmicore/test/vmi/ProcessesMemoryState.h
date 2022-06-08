#ifndef VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
#define VMICORE_PROCESSESMEMORYSTATEFIXTURE_H

#include "../../src/os/PagingDefinitions.h"
#include "../../src/os/windows/ActiveProcessesSupervisor.h"
#include "../../src/os/windows/KernelOffsets.h"
#include "../../src/os/windows/ProtectionValues.h"
#include "../../src/plugins/PluginSystem.h"
#include "../config/mock_ConfigInterface.h"
#include "../io/file/mock_LegacyLogging.h"
#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include <cstring>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::NiceMock;
using testing::Return;

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

    const processValues process0 = processValues{psActiveProcessHeadVA - _EPROCESS_OFFSETS::ActiveProcessLinks,
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
                                                 emptyFileName,
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

    std::shared_ptr<NiceMock<MockLibvmiInterface>> mockVmiInterface = std::make_shared<NiceMock<MockLibvmiInterface>>();
    std::shared_ptr<KernelAccess> kernelAccess;

    std::shared_ptr<NiceMock<MockLogging>> mockLogging = []()
    {
        std::shared_ptr<NiceMock<MockLogging>> ml = std::make_shared<NiceMock<MockLogging>>();

        ON_CALL(*ml, newNamedLogger(_))
            .WillByDefault([](const std::string& /*name*/) { return std::make_unique<NiceMock<MockGRPCLogger>>(); });

        return ml;
    }();

    std::shared_ptr<NiceMock<MockEventStream>> mockEventStream = std::make_shared<NiceMock<MockEventStream>>();

    std::shared_ptr<ActiveProcessesSupervisor> activeProcessesSupervisor;

    void setupReturnsForVmiInterface()
    {
        ON_CALL(*mockVmiInterface, getSystemCr3()).WillByDefault(Return(systemCR3));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_KPROCESS", "DirectoryTableBase"))
            .WillByDefault(Return(_KPROCESS_OFFSETS::DirectoryTableBase));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "InheritedFromUniqueProcessId"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::InheritedFromUniqueProcessId));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ImageFileName"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::ImageFileName));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "UniqueProcessId"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::UniqueProcessId));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ActiveProcessLinks"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::ActiveProcessLinks));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ExitStatus"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::ExitStatus));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "SectionObject"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::SectionObject));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "VadRoot"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::VadRoot));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EPROCESS", "ImageFilePointer"))
            .WillByDefault(Return(_EPROCESS_OFFSETS::ImageFilePointer));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_SECTION", "u1"))
            .WillByDefault(Return(_SECTION_OFFSETS::ControlArea));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_CONTROL_AREA", "u"))
            .WillByDefault(Return(_CONTROL_AREA_OFFSETS::MMSECTION_FLAGS));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_CONTROL_AREA", "FilePointer"))
            .WillByDefault(Return(_CONTROL_AREA_OFFSETS::FilePointer));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_FILE_OBJECT", "FileName"))
            .WillByDefault(Return(_FILE_OBJECT_OFFSETS::FileName));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("__MMVAD_SHORT", "VadNode"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::VadNode));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "StartingVpn"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::StartingVpn));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "StartingVpnHigh"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::StartingVpnHigh));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "EndingVpn"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::EndingVpn));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "EndingVpnHigh"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::EndingVpnHigh));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD_SHORT", "u"))
            .WillByDefault(Return(__MMVAD_SHORT_OFFSETS::Flags));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD", "Core"))
            .WillByDefault(Return(_MMVAD_OFFSETS::BaseAddress));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_MMVAD", "Subsection"))
            .WillByDefault(Return(_MMVAD_OFFSETS::Subsection));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_SUBSECTION", "ControlArea"))
            .WillByDefault(Return(_SUBSECTION_OFFSETS::ControlArea));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_EX_FAST_REF", "Object"))
            .WillByDefault(Return(_EX_FAST_REF_OFFSETS::Object));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_RTL_BALANCED_NODE", "Left"))
            .WillByDefault(Return(_RTL_BALANCED_NODE_OFFSETS::Left));
        ON_CALL(*mockVmiInterface, getKernelStructOffset("_RTL_BALANCED_NODE", "Right"))
            .WillByDefault(Return(_RTL_BALANCED_NODE_OFFSETS::Right));
        ON_CALL(*mockVmiInterface, isInitialized()).WillByDefault(Return(true));
        ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "BeingDeleted"))
            .WillByDefault(Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::beingDeleted, 1)));
        ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "Image"))
            .WillByDefault(Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::image, 6)));
        ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMSECTION_FLAGS", "File"))
            .WillByDefault(Return(std::make_tuple(0, SECTION_FLAGS_OFFSETS::file, 8)));
        ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMVAD_FLAGS", "Protection"))
            .WillByDefault(Return(std::make_tuple(0, MMVAD_FLAGS_OFFSETS::protection, 12)));
        ON_CALL(*mockVmiInterface, getBitfieldOffsetAndSizeFromJson("_MMVAD_FLAGS", "PrivateMemory"))
            .WillByDefault(Return(std::make_tuple(0, MMVAD_FLAGS_OFFSETS::privateMemory, 21)));
        ON_CALL(*mockVmiInterface, getStructSizeFromJson(KernelStructOffsets::mmvad_flags::structName))
            .WillByDefault(Return(4));
        ON_CALL(*mockVmiInterface, getStructSizeFromJson(KernelStructOffsets::mmsection_flags::structName))
            .WillByDefault(Return(4));
    }

    void setupProcessWithLink(const processValues& process, uint64_t linkEprocessBase)
    {
        ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::ExitStatus, systemCR3))
            .WillByDefault(Return(process.exitStatus));
        ON_CALL(*mockVmiInterface, read64VA(process.eprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks, systemCR3))
            .WillByDefault(Return(linkEprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks));
        ON_CALL(*mockVmiInterface,
                extractStringAtVA(process.eprocessBase + _EPROCESS_OFFSETS::ImageFileName, systemCR3))
            .WillByDefault([process = process](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(process.imageFileName); });
        ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::UniqueProcessId, systemCR3))
            .WillByDefault(Return(process.processId));
        ON_CALL(*mockVmiInterface, read64VA(process.eprocessBase + _KPROCESS_OFFSETS::DirectoryTableBase, systemCR3))
            .WillByDefault(Return(process.directoryTableBase));
    }

    void setupActiveProcessList(const std::vector<processValues>& processes)
    {
        uint64_t psActiveProcessHeadVAReturn = processes[0].eprocessBase + _EPROCESS_OFFSETS::ActiveProcessLinks;
        ON_CALL(*mockVmiInterface, translateKernelSymbolToVA("PsActiveProcessHead"))
            .WillByDefault(Return(psActiveProcessHeadVAReturn));

        for (auto process = processes.cbegin(); process != processes.cend()--; process++)
        {
            setupProcessWithLink(*process, std::next(process)->eprocessBase);
        }
        setupProcessWithLink(processes.back(), processes.begin()->eprocessBase);
    }

    void setupExtractProcessPathReturns(const processValues& process)
    {
        ON_CALL(*mockVmiInterface, read64VA(process.eprocessBase + _EPROCESS_OFFSETS::SectionObject, systemCR3))
            .WillByDefault(Return(process.sectionAddress));
        ON_CALL(*mockVmiInterface, read64VA(process.sectionAddress + _SECTION_OFFSETS::ControlArea, systemCR3))
            .WillByDefault(Return(process.controlAreaAddress));
        ON_CALL(*mockVmiInterface,
                read64VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
            .WillByDefault([processSectionFlags = process.sectionFlags](uint64_t, uint64_t)
                           { return processSectionFlags; });
        ON_CALL(*mockVmiInterface,
                read32VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
            .WillByDefault([processSectionFlags = process.sectionFlags](uint64_t, uint64_t)
                           { return processSectionFlags; });
        ON_CALL(*mockVmiInterface, read64VA(process.controlAreaAddress + _CONTROL_AREA_OFFSETS::FilePointer, systemCR3))
            .WillByDefault(Return(process.filePointerAddress));
        ON_CALL(*mockVmiInterface,
                extractUnicodeStringAtVA(process.filePointerAddress + _FILE_OBJECT_OFFSETS::FileName, systemCR3))
            .WillByDefault([processFilePath = process.filePath](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(processFilePath); });
    }

    void setupExtractProcessInformationReturns(const processValues& process)
    {
        ON_CALL(*mockVmiInterface, read64VA(process.eprocessBase + _KPROCESS_OFFSETS::DirectoryTableBase, systemCR3))
            .WillByDefault(Return(process.cr3));
        ON_CALL(*mockVmiInterface, read32VA(process.eprocessBase + _EPROCESS_OFFSETS::UniqueProcessId, systemCR3))
            .WillByDefault(Return(process.processId));
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
    std::shared_ptr<NiceMock<MockConfigInterface>> mockConfigInterface =
        std::make_shared<NiceMock<MockConfigInterface>>();
    std::shared_ptr<NiceMock<MockLegacyLogging>> mockLegacyLogging = std::make_shared<NiceMock<MockLegacyLogging>>();

    std::shared_ptr<PluginSystem> pluginSystem;

    void setupReturnsForConfigInterface()
    {
        ON_CALL(*mockConfigInterface, getPluginDirectory()).WillByDefault(Return(pluginDirectory));
    }

    uint64_t vadRootNodeBase = 666 + PagingDefinitions::kernelspaceLowerBoundary;
    uint64_t vadRootNodeRightChildBase = 777 + PagingDefinitions::kernelspaceLowerBoundary;
    uint64_t vadRootNodeLeftChildBase = 999 + PagingDefinitions::kernelspaceLowerBoundary;

    uint32_t vadRootNodeStartingVpn = 333;
    uint32_t vadRootNodeStartingVpnHigh = 0;
    uint32_t vadRootNodeEndingVpn = 334;
    uint32_t vadRootNodeEndingVpnHigh = 0;
    uint64_t vadRootNodeStartingAddress = vadRootNodeStartingVpn << PagingDefinitions::numberOfPageIndexBits;
    uint64_t vadRootNodeEndingAddress = ((vadRootNodeEndingVpn + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
    uint64_t vadRootNodeMemoryRegionSize = vadRootNodeEndingAddress - vadRootNodeStartingAddress + 1;
    ProtectionValues vadRootNodeMemoryRegionProtection = ProtectionValues::PAGE_READWRITE;
    Plugin::MemoryRegion expectedMemoryRegion1{vadRootNodeStartingAddress,
                                               vadRootNodeMemoryRegionSize,
                                               std::string{},
                                               vadRootNodeMemoryRegionProtection,
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
    ProtectionValues vadRootNodeChildMemoryRegionProtection = ProtectionValues::PAGE_EXECUTE_WRITECOPY;
    Plugin::MemoryRegion expectedMemoryRegion2{vadRootNodeRightChildStartingAddress,
                                               vadRootNodeChildMemoryRegionSize,
                                               fileNameString,
                                               vadRootNodeChildMemoryRegionProtection,
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
    Plugin::MemoryRegion expectedMemoryRegion3{vadRootNodeLeftChildStartingAddress,
                                               vadRootNodeLeftChildMemoryRegionSize,
                                               std::string{},
                                               ProtectionValues::PAGE_READWRITE,
                                               false,
                                               false,
                                               false};

    void systemVadTreeRootNodeMemoryState()
    {
        ON_CALL(*mockVmiInterface, read64VA(process4.eprocessBase + _EPROCESS_OFFSETS::VadRoot, systemCR3))
            .WillByDefault(Return(vadRootNodeBase));
        ON_CALL(*mockVmiInterface,
                read64VA(vadRootNodeBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                             _RTL_BALANCED_NODE_OFFSETS::Left,
                         systemCR3))
            .WillByDefault(Return(vadRootNodeLeftChildBase));
        ON_CALL(*mockVmiInterface,
                read64VA(vadRootNodeBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                             _RTL_BALANCED_NODE_OFFSETS::Right,
                         systemCR3))
            .WillByDefault(Return(vadRootNodeRightChildBase));
        ON_CALL(*mockVmiInterface, read8VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
            .WillByDefault(Return(vadRootNodeStartingVpnHigh));
        ON_CALL(*mockVmiInterface, read8VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
            .WillByDefault(Return(vadRootNodeEndingVpnHigh));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeStartingVpn));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeEndingVpn));
        ON_CALL(*mockVmiInterface, read64VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE), true)));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE), true)));
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
            .WillByDefault(Return(0));
        ON_CALL(*mockVmiInterface,
                read64VA(vadRootNodeRightChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                             _RTL_BALANCED_NODE_OFFSETS::Right,
                         systemCR3))
            .WillByDefault(Return(0));
        ON_CALL(*mockVmiInterface,
                read8VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
            .WillByDefault(Return(0));
        ON_CALL(*mockVmiInterface, read8VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
            .WillByDefault(Return(0));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeRightChildStartingVpn));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeRightChildEndingVpn));
        ON_CALL(*mockVmiInterface, read64VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(
                Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY), false)));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeRightChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(
                Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_EXECUTE_WRITECOPY), false)));
        ON_CALL(*mockVmiInterface, read64VA(vadRootNodeRightChildBase + _MMVAD_OFFSETS::Subsection, systemCR3))
            .WillByDefault(Return(subsectionAddress));
        ON_CALL(*mockVmiInterface, read64VA(subsectionAddress + _SUBSECTION_OFFSETS::ControlArea, systemCR3))
            .WillByDefault(Return(controlAreaAddress));
        ON_CALL(*mockVmiInterface, read64VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
            .WillByDefault(Return(process4.sectionFlags));
        ON_CALL(*mockVmiInterface, read32VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::MMSECTION_FLAGS, systemCR3))
            .WillByDefault(Return(process4.sectionFlags));
        ON_CALL(
            *mockVmiInterface,
            read64VA(controlAreaAddress + _CONTROL_AREA_OFFSETS::FilePointer + _EX_FAST_REF_OFFSETS::Object, systemCR3))
            .WillByDefault(Return(filePointerObjectAddress));
        ON_CALL(*mockVmiInterface,
                extractUnicodeStringAtVA((filePointerObjectAddress) + _FILE_OBJECT_OFFSETS::FileName, systemCR3))
            .WillByDefault([fileNameString = fileNameString](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(fileNameString); });
        ON_CALL(*mockVmiInterface, read64VA(process4.eprocessBase + _EPROCESS_OFFSETS::ImageFilePointer, systemCR3))
            .WillByDefault(Return(filePointerObjectAddress));
    }

    void systemVadTreeLeftChildOfRootNodeMemoryState()
    {
        ON_CALL(*mockVmiInterface,
                read64VA(vadRootNodeLeftChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                             _RTL_BALANCED_NODE_OFFSETS::Left,
                         systemCR3))
            .WillByDefault(Return(0));
        ON_CALL(*mockVmiInterface,
                read64VA(vadRootNodeLeftChildBase + _MMVAD_OFFSETS::BaseAddress + __MMVAD_SHORT_OFFSETS::VadNode +
                             _RTL_BALANCED_NODE_OFFSETS::Right,
                         systemCR3))
            .WillByDefault(Return(vadRootNodeBase));
        ON_CALL(*mockVmiInterface,
                read8VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::StartingVpnHigh, systemCR3))
            .WillByDefault([vadLeftChildStartingVpn = vadRootNodeLeftChildStartingVpn](uint64_t, uint64_t)
                           { return vadLeftChildStartingVpn >> 32; });
        ON_CALL(*mockVmiInterface, read8VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::EndingVpnHigh, systemCR3))
            .WillByDefault([vadLeftChildEndingVpn = vadRootNodeLeftChildEndingVpn](uint64_t, uint64_t)
                           { return vadLeftChildEndingVpn >> 32; });
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::StartingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeLeftChildStartingVpn));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::EndingVpn, systemCR3))
            .WillByDefault(Return(vadRootNodeLeftChildEndingVpn));
        ON_CALL(*mockVmiInterface, read64VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE), true)));
        ON_CALL(*mockVmiInterface, read32VA(vadRootNodeLeftChildBase + __MMVAD_SHORT_OFFSETS::Flags, systemCR3))
            .WillByDefault(Return(createMmvadFlags(static_cast<uint32_t>(ProtectionValues::PAGE_READWRITE), true)));
    }

    static uint32_t createMmvadFlags(uint32_t protection, bool privateMemory)
    {
        uint32_t flags = protection << MMVAD_FLAGS_OFFSETS::protection | privateMemory
                                                                             << MMVAD_FLAGS_OFFSETS::privateMemory;
        return flags;
    }

    static uint32_t createSectionFlags(bool image, bool beingDeleted, bool file)
    {
        uint64_t flags = image << SECTION_FLAGS_OFFSETS::image | beingDeleted << SECTION_FLAGS_OFFSETS::beingDeleted |
                         file << SECTION_FLAGS_OFFSETS::file;
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

        kernelAccess = std::make_shared<KernelAccess>(mockVmiInterface);
        activeProcessesSupervisor =
            std::make_shared<ActiveProcessesSupervisor>(mockVmiInterface, kernelAccess, mockLogging, mockEventStream);
        pluginSystem = std::make_shared<PluginSystem>(mockConfigInterface,
                                                      mockVmiInterface,
                                                      activeProcessesSupervisor,
                                                      mockLegacyLogging,
                                                      mockLogging,
                                                      std::make_shared<NiceMock<MockEventStream>>());
    };
};

#endif // VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
