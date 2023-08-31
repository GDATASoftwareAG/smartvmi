#include "FakeYara.h"
#include "mock_Config.h"
#include "mock_Dumping.h"
#include "mock_Yara.h"
#include <Scanner.h>
#include <fmt/core.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vmicore_test/io/mock_Logger.h>
#include <vmicore_test/os/mock_MemoryRegionExtractor.h>
#include <vmicore_test/os/mock_PageProtection.h>
#include <vmicore_test/plugins/mock_PluginInterface.h>

using testing::_;
using testing::An;
using testing::AnyNumber;
using testing::ByMove;
using testing::ContainsRegex;
using testing::NiceMock;
using testing::Return;
using testing::Unused;
using VmiCore::ActiveProcessInformation;
using VmiCore::MemoryRegion;
using VmiCore::MockLogger;
using VmiCore::MockMemoryRegionExtractor;
using VmiCore::MockPageProtection;
using VmiCore::Plugin::MockPluginInterface;

namespace InMemoryScanner
{
    class ScannerTestBaseFixture : public testing::Test
    {
      protected:
        const size_t maxScanSize = 0x3200000;
        const pid_t testPid = 4;
        const pid_t processIdWithSharedBaseImageRegion = 5;

        std::unique_ptr<MockPluginInterface> pluginInterface = std::make_unique<MockPluginInterface>();
        std::shared_ptr<MockConfig> configuration = std::make_shared<MockConfig>();
        std::optional<Scanner> scanner;
        MockMemoryRegionExtractor* systemMemoryRegionExtractorRaw = nullptr;
        MockMemoryRegionExtractor* sharedBaseImageMemoryRegionExtractorRaw = nullptr;

        std::string uidRegEx = "[0-9]+";
        std::string protectionAsString = "RWX";

        std::filesystem::path inMemoryDumpsPath = "inMemDumps";
        std::filesystem::path dumpedRegionsPath = inMemoryDumpsPath / "dumpedRegions";
        VmiCore::addr_t startAddress = 0x1234000;
        size_t size = 0x666;

        std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>> runningProcesses;

        void SetUp() override
        {
            ON_CALL(*pluginInterface, newNamedLogger(_))
                .WillByDefault([]() { return std::make_unique<NiceMock<MockLogger>>(); });
            ON_CALL(*pluginInterface, getResultsDir()).WillByDefault([]() { return std::make_unique<std::string>(); });
            ON_CALL(*configuration, getMaximumScanSize()).WillByDefault(Return(maxScanSize));
            // make sure that we return a non-empty memory region or else we might skip important parts
            ON_CALL(*pluginInterface, readProcessMemoryRegion(_, _, _))
                .WillByDefault([]() { return std::make_unique<std::vector<uint8_t>>(6, 9); });

            ON_CALL(*configuration, getOutputPath())
                .WillByDefault([inMemoryDumpsPath = inMemoryDumpsPath]() { return inMemoryDumpsPath; });
            ON_CALL(*configuration, getMaximumScanSize()).WillByDefault(Return(maxScanSize));

            runningProcesses = std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>();
            auto m1 = std::make_unique<MockMemoryRegionExtractor>();
            systemMemoryRegionExtractorRaw = m1.get();
            runningProcesses->push_back(std::make_shared<ActiveProcessInformation>(
                ActiveProcessInformation{0,
                                         0,
                                         0,
                                         testPid,
                                         0,
                                         "System.exe",
                                         std::make_unique<std::string>("System.exe"),
                                         std::make_unique<std::string>(""),
                                         std::move(m1),
                                         false}));
            auto m2 = std::make_unique<MockMemoryRegionExtractor>();
            sharedBaseImageMemoryRegionExtractorRaw = m2.get();
            runningProcesses->push_back(std::make_shared<ActiveProcessInformation>(
                ActiveProcessInformation{0,
                                         0,
                                         0,
                                         processIdWithSharedBaseImageRegion,
                                         0,
                                         "SomeProcess.exe",
                                         std::make_unique<std::string>("SomeProcess.exe"),
                                         std::make_unique<std::string>(""),
                                         std::move(m2),
                                         false}));
        };

        std::shared_ptr<const ActiveProcessInformation> getProcessInfoFromRunningProcesses(pid_t pid)
        {
            return *std::find_if(runningProcesses->cbegin(),
                                 runningProcesses->cend(),
                                 [pid = pid](const std::shared_ptr<const ActiveProcessInformation>& a)
                                 { return a->pid == pid; });
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
        MemoryRegion memoryRegionDescriptor{
            startAddress, size, "", std::make_unique<MockPageProtection>(), false, false, false};
        MemoryRegion memoryRegionDescriptorForSharedMemory{
            startAddress, size, "", std::make_unique<MockPageProtection>(), true, false, true};
        const std::string uidFirstRegion = "0";

        void SetUp() override
        {
            ScannerTestBaseFixture::SetUp();

            ON_CALL(*configuration, isDumpingMemoryActivated()).WillByDefault(Return(true));
            ON_CALL(*dynamic_cast<MockPageProtection*>(memoryRegionDescriptor.protection.get()), toString())
                .WillByDefault(Return(protectionAsString));
            ON_CALL(*systemMemoryRegionExtractorRaw, extractAllMemoryRegions())
                .WillByDefault(
                    [&memoryRegionDescriptor = memoryRegionDescriptor]()
                    {
                        auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                        memoryRegions->push_back(std::move(memoryRegionDescriptor));
                        return memoryRegions;
                    });
            ON_CALL(*sharedBaseImageMemoryRegionExtractorRaw, extractAllMemoryRegions())
                .WillByDefault(
                    [&memoryRegionDescriptorForSharedMemory = memoryRegionDescriptorForSharedMemory]()
                    {
                        auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                        memoryRegions->push_back(std::move(memoryRegionDescriptorForSharedMemory));
                        return memoryRegions;
                    });
            auto dumping = std::make_unique<Dumping>(pluginInterface.get(), configuration);
            auto yara = std::make_unique<NiceMock<MockYara>>();
            ON_CALL(*yara, scanMemory(_)).WillByDefault([]() { return std::make_unique<std::vector<Rule>>(); });
            scanner.emplace(pluginInterface.get(), configuration, std::move(yara), std::move(dumping));
        };

        std::string getMemFileName(std::string& trimmedProcessName, pid_t pid)
        {
            return fmt::format("{}-{}-{}-{:x}-{:x}-{}",
                               trimmedProcessName,
                               pid,
                               protectionAsString,
                               startAddress,
                               startAddress + size,
                               uidFirstRegion);
        }
    };

    TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_largeMemoryRegion_trimToMaxScanSize)
    {
        ON_CALL(*systemMemoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [startAddress = startAddress, maxScanSize = maxScanSize]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->emplace_back(
                        startAddress, maxScanSize + 1, "", std::make_unique<MockPageProtection>(), false, false, false);
                    return memoryRegions;
                });

        EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAddress, maxScanSize))
            .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
        EXPECT_NO_THROW(scanner->scanProcess(getProcessInfoFromRunningProcesses(testPid)));
    }

    TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_smallMemoryRegion_originalReadMemoryRegionSize)
    {
        ON_CALL(*systemMemoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [startAddress = startAddress, size = size]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->emplace_back(
                        startAddress, size, "", std::make_unique<MockPageProtection>(), false, false, false);
                    return memoryRegions;
                });

        EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAddress, size))
            .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
        EXPECT_NO_THROW(scanner->scanProcess(getProcessInfoFromRunningProcesses(testPid)));
    }

    TEST_F(ScannerTestFixtureDumpingDisabled, scanProcess_disabledDumping_dumpingNotCalled)
    {
        ON_CALL(*systemMemoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [startAddress = startAddress, size = size]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->emplace_back(
                        startAddress, size, "", std::make_unique<MockPageProtection>(), false, false, false);
                    return memoryRegions;
                });
        EXPECT_CALL(*pluginInterface, readProcessMemoryRegion(testPid, startAddress, size))
            .WillOnce(Return(ByMove(std::make_unique<std::vector<uint8_t>>())));
        EXPECT_CALL(*dumpingRawPointer, dumpMemoryRegion(_, _, _, _)).Times(0);

        EXPECT_NO_THROW(scanner->scanProcess(getProcessInfoFromRunningProcesses(testPid)));
    }

    TEST_F(ScannerTestFixtureDumpingEnabled, scanProcess_processWithLongName_memoryDumpWrittenWithTrimmedName)
    {
        std::string fullProcessName = "abcdefghijklmnopqrstuvwxyz!1!";
        std::string trimmedProcessName = "abcdefghijklmn";
        auto pid = 123;
        auto memoryRegionExtractor = std::make_unique<MockMemoryRegionExtractor>();
        auto* memoryRegionExtractorRaw = memoryRegionExtractor.get();
        auto processWithLongName = std::make_shared<const ActiveProcessInformation>(
            ActiveProcessInformation{0,
                                     0,
                                     0,
                                     pid,
                                     0,
                                     trimmedProcessName,
                                     std::make_unique<std::string>(fullProcessName),
                                     std::make_unique<std::string>(),
                                     std::move(memoryRegionExtractor),
                                     false});
        // Redefine default mock return because a new MemoryRegionExtractor mock has been created
        ON_CALL(*memoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [&memoryRegionDescriptor = memoryRegionDescriptor]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->push_back(std::move(memoryRegionDescriptor));
                    return memoryRegions;
                });
        auto expectedFileNameRegEx = fmt::format("{}-{}-{}-{:x}-{:x}-{}",
                                                 trimmedProcessName,
                                                 pid,
                                                 protectionAsString,
                                                 startAddress,
                                                 startAddress + size,
                                                 uidRegEx);
        auto expectedFileNameWithPathRegEx = "^" + (dumpedRegionsPath / expectedFileNameRegEx).string() + "$";

        EXPECT_CALL(*pluginInterface,
                    writeToFile(ContainsRegex(expectedFileNameWithPathRegEx), An<const std::vector<uint8_t>&>()));
        EXPECT_NO_THROW(scanner->scanProcess(processWithLongName));
    }

    TEST_F(ScannerTestFixtureDumpingEnabled, scanProcess_shortProcessName_memoryDumpWrittenWithOriginalName)
    {
        auto processWithShortName = getProcessInfoFromRunningProcesses(testPid);
        auto expectedProcessName = processWithShortName->name;
        auto expectedFileNameRegEx = fmt::format("{}-{}-{}-{:x}-{:x}-{}",
                                                 expectedProcessName,
                                                 testPid,
                                                 protectionAsString,
                                                 startAddress,
                                                 startAddress + size,
                                                 uidRegEx);
        auto expectedFileNameWithPathRegEx = "^" + (dumpedRegionsPath / expectedFileNameRegEx).string() + "$";

        EXPECT_CALL(*pluginInterface,
                    writeToFile(ContainsRegex(expectedFileNameWithPathRegEx), An<const std::vector<uint8_t>&>()));
        EXPECT_NO_THROW(scanner->scanProcess(processWithShortName));
    }

    TEST_F(ScannerTestFixtureDumpingDisabled,
           scanAllProcesses_MoreScanningThreadThanAllowedByYara_ThreadLimitNotExceeded)
    {
        auto yaraFake = std::make_unique<FakeYara>();
        auto* yaraFakeRaw = yaraFake.get();
        scanner.emplace(
            pluginInterface.get(), configuration, std::move(yaraFake), std::make_unique<NiceMock<MockDumping>>());
        auto memoryRegionExtractor = std::make_unique<MockMemoryRegionExtractor>();
        auto* memoryRegionExtractorRaw = memoryRegionExtractor.get();
        auto processInfo = std::make_shared<ActiveProcessInformation>(
            ActiveProcessInformation{0,
                                     0,
                                     0,
                                     testPid,
                                     0,
                                     "System.exe",
                                     std::make_unique<std::string>("System.exe"),
                                     std::make_unique<std::string>(""),
                                     std::move(memoryRegionExtractor),
                                     false});
        ON_CALL(*pluginInterface, getRunningProcesses())
            .WillByDefault(
                [&processInfo]()
                {
                    return std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>(
                        YR_MAX_THREADS + 5, processInfo);
                });
        ON_CALL(*memoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [startAddress = startAddress, size = size]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->emplace_back(
                        startAddress, size, "", std::make_unique<MockPageProtection>(), false, false, false);

                    return memoryRegions;
                });

        ASSERT_NO_THROW(scanner->scanAllProcesses());

        EXPECT_FALSE(yaraFakeRaw->max_threads_exceeded) << "More threads spawned than allowed by yaraFake.";
    }

    TEST_F(ScannerTestFixtureDumpingEnabled, scanAllProcesses_ProcessWithLongNameScanned_ProcessInformationWritten)
    {
        std::string fullProcessName = "abcdefghijklmnop";
        std::string trimmedProcessName = "abcdefghijklmn";
        auto pid = 123;
        auto memoryRegionExtractor = std::make_unique<MockMemoryRegionExtractor>();
        auto* memoryRegionExtractorRaw = memoryRegionExtractor.get();
        auto processInfo = std::make_shared<ActiveProcessInformation>(
            ActiveProcessInformation{0,
                                     0,
                                     0,
                                     pid,
                                     0,
                                     "",
                                     std::make_unique<std::string>(fullProcessName),
                                     std::make_unique<std::string>(""),
                                     std::move(memoryRegionExtractor),
                                     false});
        auto expectedFileName = inMemoryDumpsPath / "MemoryRegionInformation.json";
        std::string jsonStart = "{";
        std::string expectedFileContent =
            jsonStart + R"("ProcessName": ")" + fullProcessName + "\", " + "\"ProcessId\": " + std::to_string(pid) +
            ", " + "\"SharedMemory\": " + (memoryRegionDescriptor.isSharedMemory ? "true" : "false") + ", " +
            R"("AccessRights": ")" + protectionAsString + "\", " + R"("StartAddress": ")" +
            fmt::format("{:x}", startAddress) + "\", " + R"("EndAddress": ")" +
            fmt::format("{:x}", startAddress + size) + "\", " +
            "\"BeingDeleted\": " + (memoryRegionDescriptor.isBeingDeleted ? "true" : "false") + ", " +
            "\"ProcessBaseImage\": " + (memoryRegionDescriptor.isProcessBaseImage ? "true" : "false") + ", " +
            "\"Uid\": " + uidFirstRegion + ", " + R"("DumpFileName": ")" + getMemFileName(trimmedProcessName, pid) +
            "\" " + "}";
        ON_CALL(*pluginInterface, getRunningProcesses())
            .WillByDefault(
                [&processInfo]() {
                    return std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>(1,
                                                                                                          processInfo);
                });
        // Redefine default mock return because a new MemoryRegionExtractor mock has been created
        ON_CALL(*memoryRegionExtractorRaw, extractAllMemoryRegions())
            .WillByDefault(
                [&memoryRegionDescriptor = memoryRegionDescriptor]()
                {
                    auto memoryRegions = std::make_unique<std::list<MemoryRegion>>();
                    memoryRegions->push_back(std::move(memoryRegionDescriptor));

                    return memoryRegions;
                });

        EXPECT_CALL(*pluginInterface, writeToFile(_, An<const std::string&>())).Times(AnyNumber());
        EXPECT_CALL(*pluginInterface, writeToFile(expectedFileName.string(), expectedFileContent + "\n")).Times(1);

        ASSERT_NO_THROW(scanner->scanAllProcesses());
        ASSERT_NO_THROW(scanner->saveOutput());
    }
}
