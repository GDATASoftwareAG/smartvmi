#include "../src/Scanner.h"
#include "FakeYara.h"
#include "mock_Config.h"
#include "mock_Dumping.h"
#include "mock_PluginInterface.h"
#include "mock_Yara.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::An;
using testing::ByMove;
using testing::ContainsRegex;
using testing::NiceMock;
using testing::Return;
using testing::Unused;

class ScannerTestBaseFixture : public testing::Test
{
  protected:
    size_t maxScanSize = 0x3200000;
    pid_t testPid = 4;

    std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<MockPluginInterface>();
    std::shared_ptr<MockConfig> configuration = std::make_shared<MockConfig>();
    std::optional<Scanner> scanner;

    std::string uidRegEx = "[0-9]+";
    std::string vmiResultsOutputDir = "vmiResultsOutput";
    std::string inMemOutputDir = "inMemDumps";
    std::string dumpedRegionsDir = "dumpedRegions";

    std::string protectionFlagsString = "RWX";
    ProtectionValues protectionFlags = ProtectionValues::PAGE_GUARD_PAGE_EXECUTE_READWRITE;

    std::filesystem::path inMemoryDumpsPath = std::filesystem::path(vmiResultsOutputDir) / inMemOutputDir;
    std::filesystem::path dumpedRegionsPath = inMemoryDumpsPath / dumpedRegionsDir;
    virtual_address_t startAdress = 0x1234000;
    size_t size = 0x666;

    void SetUp() override
    {
        ON_CALL(*pluginInterface, getResultsDir()).WillByDefault([]() { return std::make_unique<std::string>(); });
        ON_CALL(*configuration, getMaximumScanSize()).WillByDefault(Return(maxScanSize));
        // make sure that we return a non-empty memory region or else we might skip important parts
        ON_CALL(*pluginInterface, readProcessMemoryRegion(_, _, _))
            .WillByDefault([]() { return std::make_unique<std::vector<uint8_t>>(6, 9); });

        ON_CALL(*pluginInterface, getResultsDir())
            .WillByDefault([vmiResultsOutputDir = vmiResultsOutputDir]()
                           { return std::make_unique<std::string>(vmiResultsOutputDir); });
        ON_CALL(*configuration, getOutputPath())
            .WillByDefault([inMemOutputDir = inMemOutputDir]() { return std::string(inMemOutputDir + "/"); });
        ON_CALL(*configuration, getMaximumScanSize()).WillByDefault(Return(maxScanSize));
    };
};

class ScannerTestFixtureDumpingDisabled : public ScannerTestBaseFixture
{
  protected:
    MockDumping* dumpingRawPointer{};

    void SetUp() override
    {
        ScannerTestBaseFixture::SetUp();

        ON_CALL(*configuration, isDumpingMemoryActivated()).WillByDefault(Return(false));
        auto dumping = std::make_unique<NiceMock<MockDumping>>();
        dumpingRawPointer = dumping.get();
        scanner.emplace(pluginInterface.get(), configuration, std::unique_ptr<YaraInterface>{}, std::move(dumping));
    };
};

class ScannerTestFixtureDumpingEnabled : public ScannerTestBaseFixture
{
  protected:
    Plugin::MemoryRegion memoryRegionDescriptor{startAdress, size, "", protectionFlags, false, false, false};
    Plugin::MemoryRegion memoryRegionDescriptorForSharedMemory{
        startAdress, size, "", protectionFlags, true, false, true};
    const std::string uidFirstRegion = "0";
    const pid_t processIdWithSharedBaseImageRegion = 5;

    void SetUp() override
    {
        ScannerTestBaseFixture::SetUp();

        ON_CALL(*configuration, isDumpingMemoryActivated()).WillByDefault(Return(true));
        ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
            .WillByDefault(
                [&memoryRegionDescriptor = memoryRegionDescriptor](Unused)
                {
                    auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                    memoryRegions->push_back(memoryRegionDescriptor);
                    return memoryRegions;
                });
        ON_CALL(*pluginInterface, getProcessMemoryRegions(processIdWithSharedBaseImageRegion))
            .WillByDefault(
                [&memoryRegionDescriptorForSharedMemory = memoryRegionDescriptorForSharedMemory](Unused)
                {
                    auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                    memoryRegions->push_back(memoryRegionDescriptorForSharedMemory);
                    return memoryRegions;
                });
        auto dumping = std::make_unique<Dumping>(pluginInterface.get(), configuration);
        auto yara = std::make_unique<NiceMock<MockYara>>();
        ON_CALL(*yara, scanMemory(_)).WillByDefault([]() { return std::make_unique<std::vector<Rule>>(); });
        scanner.emplace(pluginInterface.get(), configuration, std::move(yara), std::move(dumping));
    };

    std::string getMemFileName(std::string& trimmedProcessName, pid_t pid)
    {
        return trimmedProcessName + "-" + std::to_string(pid) + "-" + protectionFlagsString + "-" +
               intToHex(startAdress) + "-" + intToHex(startAdress + size) + "-" + uidFirstRegion;
    }
};

TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_largeMemoryRegion_trimToMaxScanSize)
{
    ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
        .WillByDefault(
            [startAdress = startAdress, maxScanSize = maxScanSize, protectionFlags = protectionFlags](Unused)
            {
                auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                memoryRegions->emplace_back(startAdress, maxScanSize + 1, "", protectionFlags, false, false, false);
                return memoryRegions;
            });

    EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAdress, maxScanSize))
        .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
    EXPECT_NO_THROW(scanner->scanProcess(testPid, "System.exe"));
}

TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_smallMemoryRegion_originalReadMemoryRegionSize)
{
    ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
        .WillByDefault(
            [startAdress = startAdress, size = size, protectionFlags = protectionFlags](Unused)
            {
                auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                memoryRegions->emplace_back(startAdress, size, "", protectionFlags, false, false, false);
                return memoryRegions;
            });

    EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAdress, size))
        .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
    EXPECT_NO_THROW(scanner->scanProcess(testPid, "System.exe"));
}

TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_disabledDumping_dumpingNotCalled)
{
    ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
        .WillByDefault(
            [startAddress = startAdress, size = size, protectionFlags = protectionFlags](Unused)
            {
                auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                memoryRegions->emplace_back(startAddress, size, "", protectionFlags, false, false, false);
                return memoryRegions;
            });

    EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAdress, size))
        .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
    EXPECT_CALL(*dumpingRawPointer, dumpMemoryRegion(_, _, _, _)).Times(0);
    EXPECT_NO_THROW(scanner->scanProcess(testPid, "System.exe"));
}

TEST_F(ScannerTestFixtureDumpingEnabled, scanProcess_processWithLongName_memoryDumpWrittenWithTrimmedName)
{
    std::string fullProcessName = "abcdefghijklmnopqrstuvwxyz!1!";
    std::string trimmedProcessName = "abcdefghijklmn";
    auto expectedFileNameRegEx = trimmedProcessName + "-" + std::to_string(testPid) + "-" + protectionFlagsString +
                                 "-" + intToHex(startAdress) + "-" + intToHex(startAdress + size) + "-" + uidRegEx;
    auto expectedFileNameWithPathRegEx = "^" + (dumpedRegionsPath / expectedFileNameRegEx).string() + "$";

    EXPECT_CALL(*pluginInterface,
                writeToFile(ContainsRegex(expectedFileNameWithPathRegEx), An<const std::vector<uint8_t>&>()));
    EXPECT_NO_THROW(scanner->scanProcess(testPid, fullProcessName));
}

TEST_F(ScannerTestFixtureDumpingEnabled, scanProcess_shortProcessName_memoryDumpWrittenWithOriginalName)
{
    std::string fullProcessName = "abcdefg";
    std::string expectedProcessName = "abcdefg";
    auto expectedFileNameRegEx = expectedProcessName + "-" + std::to_string(testPid) + "-" + protectionFlagsString +
                                 "-" + intToHex(startAdress) + "-" + intToHex(startAdress + size) + "-" + uidRegEx;
    auto expectedFileNameWithPathRegEx = "^" + (dumpedRegionsPath / expectedFileNameRegEx).string() + "$";

    EXPECT_CALL(*pluginInterface,
                writeToFile(ContainsRegex(expectedFileNameWithPathRegEx), An<const std::vector<uint8_t>&>()));
    EXPECT_NO_THROW(scanner->scanProcess(testPid, fullProcessName));
}

TEST_F(ScannerTestFixtureDumpingDisabled, scanAllProcesses_MoreScanningThreadThanAllowedByYara_ThreadLimitNotExceeded)
{
    auto yaraFake = std::make_unique<FakeYara>();
    auto yaraFakeRaw = yaraFake.get();
    scanner.emplace(
        pluginInterface.get(), configuration, std::move(yaraFake), std::make_unique<NiceMock<MockDumping>>());
    auto processInfo = ProcessInformation{testPid, "System.exe"};
    ON_CALL(*pluginInterface, getRunningProcesses())
        .WillByDefault([&processInfo]()
                       { return std::make_unique<std::vector<ProcessInformation>>(YR_MAX_THREADS + 5, processInfo); });
    ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
        .WillByDefault(
            [startAddress = startAdress, size = size, protectionFlags = protectionFlags](Unused)
            {
                auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                memoryRegions->emplace_back(startAddress, size, "", protectionFlags, false, false, false);

                return memoryRegions;
            });
    ON_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAdress, size))
        .WillByDefault([](Unused, Unused, Unused) { return std::make_unique<std::vector<uint8_t>>(0x6666, 0xCC); });

    ASSERT_NO_THROW(scanner->scanAllProcesses());

    EXPECT_FALSE(yaraFakeRaw->max_threads_exceeded) << "More threads spawned than allowed by yaraFake.";
}

TEST_F(ScannerTestFixtureDumpingEnabled, scanAllProcesses_ProcessWithLongNameScanned_ProcessInformationWritten)
{
    auto yaraFake = std::make_unique<FakeYara>();
    auto dumpingFake = std::make_unique<NiceMock<MockDumping>>();
    auto dumpingFakeRaw = dumpingFake.get();
    scanner.emplace(pluginInterface.get(), configuration, std::move(yaraFake), std::move(dumpingFake));
    auto processInfo = ProcessInformation{testPid, "abcdefg"};

    std::string fullProcessName = "abcdefghijklmnop";
    std::string trimmedProcessName = "abcdefghijklmn";
    auto expectedFileName = inMemOutputDir + "/MemoryRegionInformation.json";
    std::string jsonStart = "{";
    std::string expectedFileContent =
        jsonStart + "\"ProcessName\": \"" + fullProcessName + "\", " + "\"ProcessId\": " + std::to_string(testPid) +
        ", " + "\"SharedMemory\": " + (memoryRegionDescriptor.isSharedMemory ? "true" : "false") + ", " +
        "\"AccessRights\": \"" + protectionFlagsString + "\", " + "\"StartAddress\": \"" + intToHex(startAdress) +
        "\", " + "\"EndAddress\": \"" + intToHex(startAdress + size) + "\", " +
        "\"BeingDeleted\": " + (memoryRegionDescriptor.isBeingDeleted ? "true" : "false") + ", " +
        "\"ProcessBaseImage\": " + (memoryRegionDescriptor.isProcessBaseImage ? "true" : "false") + ", " +
        "\"Uid\": " + uidFirstRegion + ", " + "\"DumpFileName\": \"" + getMemFileName(trimmedProcessName, testPid) +
        "\" " + "}";

    ON_CALL(*pluginInterface, getRunningProcesses())
        .WillByDefault([&processInfo]() { return std::make_unique<std::vector<ProcessInformation>>(1, processInfo); });
    ON_CALL(*pluginInterface, getProcessMemoryRegions(testPid))
        .WillByDefault(
            [startAddress = startAdress, size = size, protectionFlags = protectionFlags](Unused)
            {
                auto memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
                memoryRegions->emplace_back(startAddress, size, "", protectionFlags, false, false, false);
                return memoryRegions;
            });
    ON_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAdress, size))
        .WillByDefault([](Unused, Unused, Unused) { return std::make_unique<std::vector<uint8_t>>(0x6666, 0xCC); });
    ON_CALL(*dumpingFakeRaw, getAllMemoryRegionInformation())
        .WillByDefault([&expectedFileContent]() { return std::vector<std::string>{expectedFileContent}; });

    EXPECT_CALL(*pluginInterface, writeToFile(_, An<const std::string&>())).Times(1);
    EXPECT_CALL(*pluginInterface, writeToFile(std::string(expectedFileName), expectedFileContent + "\n")).Times(1);

    ASSERT_NO_THROW(scanner->scanAllProcesses());
    ASSERT_NO_THROW(scanner->saveOutput());
}
