#ifndef VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
#define VMICORE_PROCESSESMEMORYSTATEFIXTURE_H

#include "../../src/os/PagingDefinitions.h"
#include "../../src/os/windows/ActiveProcessesSupervisor.h"
#include "../../src/os/windows/KernelObjectDefinitionsWin10.h"
#include "../../src/plugins/PluginSystem.h"
#include "../config/mock_ConfigInterface.h"
#include "../io/file/mock_LegacyLogging.h"
#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include "../os/windows/mock_KernelObjectExtractorWin10.h"
#include "mock_LibvmiInterface.h"
#include <cstring>
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::NiceMock;
using testing::Return;

using psActiveProcessListEntry = struct psActiveProcessListEntry_t
{
    uint64_t eprocessBase;
    uint64_t nextProcess;
    uint64_t directoryTableBase;
    pid_t processId;
    uint32_t exitStatus;
    std::string imageFileName;
    std::string fullName;
};

class ProcessesMemoryStateFixture : public testing::Test
{
  protected:
    uint64_t psActiveProcessHeadVA = 0xfffff80140964160;
    uint64_t systemCr3 = 0x1aa000;
    uint32_t statusPending = 0x103;
    pid_t unusedPid = 0x1337;

    uint64_t process332EprocessBase = 0xffffe001721d6080;
    uint64_t process332NextProcess = psActiveProcessHeadVA;
    pid_t process332Pid = 332;
    uint64_t process332CR3 = 0x550cc000;
    uint64_t process332sectionAddress = 0xffffc0014d5d8440;
    uint64_t process332controlAreaAddress = 0xffffe0017218aa40;
    uint64_t process332FilePointerAddress = 0xffffe00172192d10;
    std::string process332FilePath = R"(\Windows\System32\csrss.exe)";
    std::string process332FullName = "csrss.exe";
    std::string process332FileName = "csrss.exe";
    _MMSECTION_FLAGS process332SectionFlags = _MMSECTION_FLAGS{};
    _UNICODE_STRING process332UnicodeFilename = _UNICODE_STRING{};

    uint64_t process248EprocessBase = 0xffffe00172048800;
    uint64_t process248NextProcess = psActiveProcessHeadVA;
    pid_t process248Pid = 248;
    uint64_t process248CR3 = 0x62061000;
    uint64_t process248sectionAddress = 0xffffc0014d174fc0;
    uint64_t process248controlAreaAddress = 0xffffe0017204e9f0;
    uint64_t process248FilePointerAddress = 0xffffe0017202a9b0;
    std::string process248FilePath = R"(\MWatcherInterpreter\MWatcherInterpreter.exe)";
    std::string process248FullName = "MWatcherInterpreter.exe";
    std::string process248FileName = "MWatcherInterp";
    _MMSECTION_FLAGS process248SectionFlags{};
    _UNICODE_STRING process248UnicodeFilename{};

    uint64_t process4EprocessBase = 0xffffe00170250700;
    uint64_t process4NextProcess = process248EprocessBase + OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks;
    pid_t process4Pid = 4;
    std::string process4FileName = "System";

    std::string emptyFileName{};
    std::string emptyFullName{};

    uint64_t process0EprocessBase = psActiveProcessHeadVA - OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks;
    uint64_t process0NextProcess = process4EprocessBase + OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks;
    uint32_t process0ExitStatus = 0x32323232;
    pid_t process0Pid = 0;
    uint64_t process0Cr3 = 0;

    const psActiveProcessListEntry process0 = psActiveProcessListEntry{process0EprocessBase,
                                                                       process0NextProcess,
                                                                       process0Cr3,
                                                                       process0Pid,
                                                                       process0ExitStatus,
                                                                       emptyFileName,
                                                                       emptyFullName};
    const psActiveProcessListEntry process4 = psActiveProcessListEntry{process4EprocessBase,
                                                                       process4NextProcess,
                                                                       systemCr3,
                                                                       process4Pid,
                                                                       statusPending,
                                                                       process4FileName,
                                                                       emptyFileName};
    const psActiveProcessListEntry process248 = psActiveProcessListEntry{process248EprocessBase,
                                                                         process248NextProcess,
                                                                         process248CR3,
                                                                         process248Pid,
                                                                         statusPending,
                                                                         process248FileName,
                                                                         process248FullName};
    const psActiveProcessListEntry process332 = psActiveProcessListEntry{process332EprocessBase,
                                                                         process332NextProcess,
                                                                         process332CR3,
                                                                         process332Pid,
                                                                         statusPending,
                                                                         process332FileName,
                                                                         process332FullName};

    std::shared_ptr<NiceMock<MockLibvmiInterface>> mockVmiInterface = std::make_shared<NiceMock<MockLibvmiInterface>>();
    std::shared_ptr<NiceMock<MockKernelObjectExtractorWin10>> mockKernelObjectExtractorWin10 =
        std::make_shared<NiceMock<MockKernelObjectExtractorWin10>>();

    std::shared_ptr<NiceMock<MockLogging>> mockLogging = []()
    {
        std::shared_ptr<NiceMock<MockLogging>> ml = std::make_shared<NiceMock<MockLogging>>();

        ON_CALL(*ml, newNamedLogger(_))
            .WillByDefault([](const std::string& /*name*/) { return std::make_unique<NiceMock<MockGRPCLogger>>(); });

        return ml;
    }();

    std::shared_ptr<NiceMock<MockEventStream>> mockEventStream = std::make_shared<NiceMock<MockEventStream>>();

    std::shared_ptr<ActiveProcessesSupervisor> activeProcessesSupervisor = std::make_shared<ActiveProcessesSupervisor>(
        mockVmiInterface, mockKernelObjectExtractorWin10, mockLogging, mockEventStream);

    void setupReturnsForVmiInterface()
    {
        ON_CALL(*mockVmiInterface, getSystemCr3()).WillByDefault(Return(systemCr3));
    }

    void setupProcessWithLink(const psActiveProcessListEntry& process, uint64_t linkEprocessBase)
    {
        ON_CALL(*mockVmiInterface,
                read32VA(process.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ExitStatus, systemCr3))
            .WillByDefault(Return(process.exitStatus));
        ON_CALL(*mockVmiInterface,
                read64VA(process.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks, systemCr3))
            .WillByDefault(Return(linkEprocessBase + OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks));
        ON_CALL(*mockVmiInterface,
                extractStringAtVA(process.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ImageFileName, systemCr3))
            .WillByDefault([process = process](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(process.imageFileName); });
        ON_CALL(*mockVmiInterface,
                read32VA(process.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::UniqueProcessId, systemCr3))
            .WillByDefault(Return(process.processId));
        ON_CALL(*mockVmiInterface,
                read64VA(process.eprocessBase + OffsetDefinitionsWin10::_KPROCESS::DirectoryTableBase, systemCr3))
            .WillByDefault(Return(process.directoryTableBase));
    }

    void setupActiveProcessList(const std::vector<psActiveProcessListEntry>& processes)
    {
        uint64_t psActiveProcessHeadVAReturn =
            processes[0].eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks;
        ON_CALL(*mockVmiInterface, translateKernelSymbolToVA("PsActiveProcessHead"))
            .WillByDefault(Return(psActiveProcessHeadVAReturn));

        for (auto process = processes.cbegin(); process != processes.cend()--; process++)
        {
            setupProcessWithLink(*process, std::next(process)->eprocessBase);
        }
        setupProcessWithLink(processes.back(), processes.begin()->eprocessBase);
    }

    void setupProcess248ExtractProcessPathReturns()
    {
        ON_CALL(*mockVmiInterface,
                read64VA(process248.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::SectionObject, systemCr3))
            .WillByDefault(Return(process248sectionAddress));
        ON_CALL(*mockVmiInterface, read64VA(process248sectionAddress + OffsetDefinitionsWin10::_SECTION::u1, systemCr3))
            .WillByDefault(Return(process248controlAreaAddress));
        ON_CALL(*mockKernelObjectExtractorWin10,
                extractMmSectionFlags(process248controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::u))
            .WillByDefault([process248SectionFlags = process248SectionFlags](uint64_t)
                           { return std::make_unique<_MMSECTION_FLAGS>(process248SectionFlags); });
        ON_CALL(*mockVmiInterface,
                read64VA(process248controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::FilePointer, systemCr3))
            .WillByDefault(Return(process248FilePointerAddress));

        ON_CALL(*mockKernelObjectExtractorWin10,
                extractUnicodeObject(process248FilePointerAddress + OffsetDefinitionsWin10::_FILE_OBJECT::FileName))
            .WillByDefault([process248UnicodeFilename = process248UnicodeFilename](uint64_t)
                           { return std::make_unique<_UNICODE_STRING>(process248UnicodeFilename); });

        ON_CALL(*mockKernelObjectExtractorWin10,
                extractWString(reinterpret_cast<uint64_t>(process248UnicodeFilename.Buffer),
                               process248UnicodeFilename.Length))
            .WillByDefault([process248FilePath = process248FilePath](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(process248FilePath); });
    }

    void setupProcess332ExtractProcessPathReturns()
    {
        ON_CALL(*mockVmiInterface,
                read64VA(process332.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::SectionObject, systemCr3))
            .WillByDefault(Return(process332sectionAddress));
        ON_CALL(*mockVmiInterface, read64VA(process332sectionAddress + OffsetDefinitionsWin10::_SECTION::u1, systemCr3))
            .WillByDefault(Return(process332controlAreaAddress));
        ON_CALL(*mockKernelObjectExtractorWin10,
                extractMmSectionFlags(process332controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::u))
            .WillByDefault([process332SectionFlags = process332SectionFlags](uint64_t)
                           { return std::make_unique<_MMSECTION_FLAGS>(process332SectionFlags); });
        ON_CALL(*mockVmiInterface,
                read64VA(process332controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::FilePointer, systemCr3))
            .WillByDefault(Return(process332FilePointerAddress));

        ON_CALL(*mockKernelObjectExtractorWin10,
                extractUnicodeObject(process332FilePointerAddress + OffsetDefinitionsWin10::_FILE_OBJECT::FileName))
            .WillByDefault([process332UnicodeFilename = process332UnicodeFilename](uint64_t)
                           { return std::make_unique<_UNICODE_STRING>(process332UnicodeFilename); });

        ON_CALL(*mockKernelObjectExtractorWin10,
                extractWString(reinterpret_cast<uint64_t>(process332UnicodeFilename.Buffer),
                               process332UnicodeFilename.Length))
            .WillByDefault([process332FilePath = process332FilePath](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(process332FilePath); });
    }

    void setupProcessSectionFlags(_MMSECTION_FLAGS& flags)
    {
        flags.BeingDeleted = 0x0;
        flags.BeingCreated = 0x0;
        flags.BeingPurged = 0x0;
        flags.NoModifiedWriting = 0x0;
        flags.FailAllIo = 0x0;
        flags.Image = 0x1;
        flags.Based = 0x1;
        flags.File = 0x1;
        flags.AttemptingDelete = 0x0;
        flags.PrefetchCreated = 0x0;
        flags.PhysicalMemory = 0x0;
        flags.CopyOnWrite = 0x1;
        flags.Reserve = 0x1;
        flags.Commit = 0x0;
        flags.NoChange = 0x1;
        flags.WasPurged = 0x1;
        flags.UserReference = 0x1;
        flags.GlobalMemory = 0x1;
        flags.DeleteOnClose = 0x1;
        flags.FilePointerNull = 0x1;
        flags.PreferredNode = 0x3f;
        flags.GlobalOnlyPerSession = 0x1;
        flags.UserWritable = 0x1;
        flags.SystemVaAllocated = 0x1;
        flags.PreferredFsCompressionBoundary = 0x1;
        flags.UsingFileExtents = 0x1;
        flags.Spare = 0x1;
    }

    void setupProcess248UnicodeFilename()
    {
        process248UnicodeFilename.Length = 0x34;
        process248UnicodeFilename.MaximumLength = 0x38;
        process248UnicodeFilename.padding = 0x0;
        process248UnicodeFilename.Buffer = reinterpret_cast<uint16_t*>(0xffffc0014d173c90);
    }

    void setupProcess332UnicodeFilename()
    {
        process332UnicodeFilename.Length = 0x36;
        process332UnicodeFilename.MaximumLength = 0x38;
        process332UnicodeFilename.padding = 0x0;
        process332UnicodeFilename.Buffer = reinterpret_cast<uint16_t*>(0xffffc0014d5d3e70);
    }

    void setupReturnsForFullProcess248Name()
    {
        setupProcessSectionFlags(process248SectionFlags);
        setupProcess248UnicodeFilename();
        setupProcess248ExtractProcessPathReturns();
    }

    void setupReturnsForFullProcess332Name()
    {
        setupProcessSectionFlags(process332SectionFlags);
        setupProcess332UnicodeFilename();
        setupProcess332ExtractProcessPathReturns();
    }

    std::filesystem::path pluginDirectory = "/var/lib/test";
    std::shared_ptr<NiceMock<MockConfigInterface>> mockConfigInterface =
        std::make_shared<NiceMock<MockConfigInterface>>();
    std::shared_ptr<NiceMock<MockLegacyLogging>> mockLegacyLogging = std::make_shared<NiceMock<MockLegacyLogging>>();

    std::shared_ptr<PluginSystem> pluginSystem =
        std::make_shared<PluginSystem>(mockConfigInterface,
                                       mockVmiInterface,
                                       activeProcessesSupervisor,
                                       mockLegacyLogging,
                                       mockLogging,
                                       std::make_shared<NiceMock<MockEventStream>>());

    void setupReturnsForConfigInterface()
    {
        ON_CALL(*mockConfigInterface, getPluginDirectory()).WillByDefault(Return(pluginDirectory));
    }

    uint64_t vadRootNodeBase = 666;
    uint64_t vadRootNodeRightChildBase = 777;
    uint64_t vadRootNodeLeftChildBase = 999;

    uint32_t vadRootNodeStartingVpn = 333;
    uint32_t vadRootNodeEndingVpn = 334;
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

    uint32_t vadRootNodeChildStartingVpn = 444;
    uint32_t vadRootNodeChildEndingVpn = 445;
    uint64_t vadRootNodeChildStartingAddress = vadRootNodeChildStartingVpn << PagingDefinitions::numberOfPageIndexBits;
    uint64_t vadRootNodeChildEndingAddress =
        ((vadRootNodeChildEndingVpn + 1) << PagingDefinitions::numberOfPageIndexBits) - 1;
    uint64_t vadRootNodeChildMemoryRegionSize = vadRootNodeChildEndingAddress - vadRootNodeChildStartingAddress + 1;
    std::string fileNameString = std::string(R"(\Windows\IAMSYSTEM.exe)");
    ProtectionValues vadRootNodeChildMemoryRegionProtection = ProtectionValues::PAGE_EXECUTE_WRITECOPY;
    Plugin::MemoryRegion expectedMemoryRegion2{vadRootNodeChildStartingAddress,
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
        ON_CALL(*mockKernelObjectExtractorWin10,
                getVadTreeRootAddress(process4.eprocessBase + OffsetDefinitionsWin10::_EPROCESS::VadRoot))
            .WillByDefault(Return(vadRootNodeBase));
        ON_CALL(*mockKernelObjectExtractorWin10, extractMmVadShort(vadRootNodeBase))
            .WillByDefault(
                [vadRootNodeRightChildBase = vadRootNodeRightChildBase,
                 vadRootNodeLeftChildBase = vadRootNodeLeftChildBase,
                 vadRootNodeStartingVpn = vadRootNodeStartingVpn,
                 vadRootNodeEndingVpn = vadRootNodeEndingVpn](uint64_t)
                {
                    auto vadShort = std::make_unique<_MMVAD_SHORT>();
                    vadShort->StartingVpn = vadRootNodeStartingVpn;
                    vadShort->StartingVpnHigh = 0;
                    vadShort->EndingVpn = vadRootNodeEndingVpn;
                    vadShort->EndingVpnHigh = 0;
                    vadShort->u.VadFlags.PrivateMemory = 1;
                    vadShort->u.VadFlags.Protection = uint32_t(ProtectionValues::PAGE_READWRITE);
                    vadShort->VadNode.balancedNodeChildren.Left =
                        reinterpret_cast<_RTL_BALANCED_NODE_t>(vadRootNodeLeftChildBase);
                    vadShort->VadNode.balancedNodeChildren.Right =
                        reinterpret_cast<_RTL_BALANCED_NODE_t>(vadRootNodeRightChildBase);
                    return vadShort;
                });
    }

    void systemVadTreeRightChildOfRootNodeMemoryState()
    {
        uint64_t subsectionAddress = 0x88800;
        uint64_t controlAreaAddress = 0x99900;
        uint64_t filePointerObjectAddress = 0x2340;
        uint64_t filePointerObjectAddressRefCount = 0x1;
        uint64_t imageFilePointerAddressLocation =
            process4EprocessBase + OffsetDefinitionsWin10::_EPROCESS::ImageFilePointer;
        uint64_t filenameBufferAddress = 0x98765;
        auto fileNameBufferLength = static_cast<uint16_t>(fileNameString.size() + 1);

        ON_CALL(*mockKernelObjectExtractorWin10, extractMmVadShort(vadRootNodeRightChildBase))
            .WillByDefault(
                [vadRootNodeChildStartingVpn = vadRootNodeChildStartingVpn,
                 vadRootNodeChildEndingVpn = vadRootNodeChildEndingVpn](uint64_t)
                {
                    auto vadShort = std::make_unique<_MMVAD_SHORT>();
                    vadShort->StartingVpn = vadRootNodeChildStartingVpn;
                    vadShort->StartingVpnHigh = 0;
                    vadShort->EndingVpn = vadRootNodeChildEndingVpn;
                    vadShort->EndingVpnHigh = 0;
                    vadShort->u.VadFlags.PrivateMemory = 0;
                    vadShort->u.VadFlags.Protection = uint32_t(ProtectionValues::PAGE_EXECUTE_WRITECOPY);
                    vadShort->VadNode.balancedNodeChildren.Left = nullptr;
                    vadShort->VadNode.balancedNodeChildren.Right = nullptr;
                    return vadShort;
                });
        ON_CALL(*mockKernelObjectExtractorWin10, extractMmVad(vadRootNodeRightChildBase))
            .WillByDefault(
                [subsectionAddress = subsectionAddress](uint64_t)
                {
                    auto vad = std::make_unique<_MMVAD>();
                    vad->Subsection = reinterpret_cast<_SUBSECTION_t>(subsectionAddress);
                    return vad;
                });
        ON_CALL(*mockKernelObjectExtractorWin10, extractMmVadSubsection(subsectionAddress))
            .WillByDefault(
                [controlAreaAddress = controlAreaAddress](uint64_t)
                {
                    auto subsection = std::make_unique<_SUBSECTION>();
                    subsection->ControlArea = reinterpret_cast<_CONTROL_AREA_t>(controlAreaAddress);
                    return subsection;
                });
        ON_CALL(*mockKernelObjectExtractorWin10, extractControlArea(controlAreaAddress))
            .WillByDefault(
                [filePointerObjectAddress = filePointerObjectAddress,
                 filePointerObjectAddressRefCount = filePointerObjectAddressRefCount](uint64_t)
                {
                    auto controlArea = std::make_unique<_CONTROL_AREA>();
                    controlArea->u.Flags.Image = 1;
                    controlArea->u.Flags.BeingDeleted = 1;
                    controlArea->FilePointer.Object =
                        reinterpret_cast<_EX_FAST_REF_t>(filePointerObjectAddress | filePointerObjectAddressRefCount);
                    return controlArea;
                });
        ON_CALL(*mockKernelObjectExtractorWin10, extractFileObject(filePointerObjectAddress))
            .WillByDefault(
                [filenameBufferAddress = filenameBufferAddress, fileNameBufferLength = fileNameBufferLength](uint64_t)
                {
                    auto fileObject = std::make_unique<_FILE_OBJECT>();
                    fileObject->FileName.Buffer = reinterpret_cast<uint16_t*>(filenameBufferAddress);
                    fileObject->FileName.Length = fileNameBufferLength;
                    return fileObject;
                });
        ON_CALL(*mockKernelObjectExtractorWin10, extractWString(filenameBufferAddress, fileNameBufferLength))
            .WillByDefault([fileNameString = fileNameString](uint64_t, uint64_t)
                           { return std::make_unique<std::string>(fileNameString); });
        ON_CALL(*mockKernelObjectExtractorWin10, getImageFilePointer(imageFilePointerAddressLocation))
            .WillByDefault(Return(filePointerObjectAddress));
    }

    void systemVadTreeLeftChildOfRootNodeMemoryState()
    {
        ON_CALL(*mockKernelObjectExtractorWin10, extractMmVadShort(vadRootNodeLeftChildBase))
            .WillByDefault(
                [vadRootNodeBase = vadRootNodeBase,
                 vadLeftChildStartingVpn = vadRootNodeLeftChildStartingVpn,
                 vadLeftChildEndingVpn = vadRootNodeLeftChildEndingVpn](uint64_t)
                {
                    auto vadShort = std::make_unique<_MMVAD_SHORT>();
                    vadShort->StartingVpn = static_cast<uint32_t>(vadLeftChildStartingVpn);
                    vadShort->StartingVpnHigh = vadLeftChildStartingVpn >> 32;
                    vadShort->EndingVpn = static_cast<uint32_t>(vadLeftChildEndingVpn);
                    vadShort->EndingVpnHigh = vadLeftChildEndingVpn >> 32;
                    vadShort->u.VadFlags.PrivateMemory = 1;
                    vadShort->u.VadFlags.Protection = uint32_t(ProtectionValues::PAGE_READWRITE);
                    vadShort->VadNode.balancedNodeChildren.Left = nullptr;
                    // Purposely create a cycle in the tree
                    vadShort->VadNode.balancedNodeChildren.Right =
                        reinterpret_cast<_RTL_BALANCED_NODE_t>(vadRootNodeBase);
                    return vadShort;
                });
    }

    void process4VadTreeMemoryState()
    {
        systemVadTreeRootNodeMemoryState();
        systemVadTreeRightChildOfRootNodeMemoryState();
        systemVadTreeLeftChildOfRootNodeMemoryState();
    }

    void SetUp() override
    {
        setupReturnsForVmiInterface();
        setupReturnsForConfigInterface();
    };
};

#endif // VMICORE_PROCESSESMEMORYSTATEFIXTURE_H
