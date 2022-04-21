#pragma once

#include "Config.h"

#include <PluginInterface.h>
#include <filesystem>
#include <mutex>
#include <random>
#include <string>
#include <utility>
#include <vector>

using ProtectionValues = KernelObjectDefinitionsWin10::ProtectionValues;

class MemoryRegionInformation
{
  public:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    const std::string pid;
    const size_t scanSize;
    const std::string flags;
    const bool isBeingDeleted;
    const bool isProcessBaseImage;
    const bool isSharedMemory;
    const std::string moduleName;
    const std::string processName;
    const std::string uid;
    const std::string startAdress;
    const std::string endAdress;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

    MemoryRegionInformation(std::string pid,
                            size_t scanSize,
                            std::string flags,
                            bool isBeingDeleted,
                            bool isProcessBaseImage,
                            bool isSharedMemory,
                            std::string moduleName,
                            std::string processName,
                            std::string uid,
                            std::string startAdress,
                            std::string endAdress)
        : pid(std::move(pid)),
          scanSize(scanSize),
          flags(std::move(flags)),
          isBeingDeleted(isBeingDeleted),
          isProcessBaseImage(isProcessBaseImage),
          isSharedMemory(isSharedMemory),
          moduleName(std::move(moduleName)),
          processName(std::move(processName)),
          uid(std::move(uid)),
          startAdress(std::move(startAdress)),
          endAdress(std::move(endAdress))
    {
    }

    std::string getMemFileName()
    {
        return processName.substr(0, maxProcNameLength)
            .append("-")
            .append(pid)
            .append("-")
            .append(flags)
            .append("-")
            .append(startAdress)
            .append("-")
            .append(endAdress)
            .append("-")
            .append(uid);
    }

    std::string toString()
    {
        return std::string("{")
            .append(R"("ProcessName": ")")
            .append(processName)
            .append(R"(", )")
            .append(R"("ProcessId": )")
            .append(pid)
            .append(", ")
            .append(R"("SharedMemory": )")
            .append(boolToString(isSharedMemory))
            .append(", ")
            .append(R"("AccessRights": ")")
            .append(flags)
            .append(R"(", )")
            .append(R"("StartAddress": ")")
            .append(startAdress)
            .append(R"(", )")
            .append(R"("EndAddress": ")")
            .append(endAdress)
            .append(R"(", )")
            .append(R"("BeingDeleted": )")
            .append(boolToString(isBeingDeleted))
            .append(", ")
            .append(R"("ProcessBaseImage": )")
            .append(boolToString(isProcessBaseImage))
            .append(", ")
            .append(R"("Uid": )")
            .append(uid)
            .append(", ")
            .append(R"("DumpFileName": ")")
            .append(getMemFileName())
            .append(R"(" })");
    }

  private:
    const int maxProcNameLength = 14;

    static std::string boolToString(bool value)
    {
        return value ? "true" : "false";
    }
};

class IDumping
{
  public:
    virtual ~IDumping() = default;

    virtual void dumpMemoryRegion(const std::string& processName,
                                  pid_t pid,
                                  const Plugin::MemoryRegion& memoryRegionDescriptor,
                                  const std::vector<uint8_t>& data) = 0;

    virtual std::vector<std::string> getAllMemoryRegionInformation() = 0;

  protected:
    IDumping() = default;
};

class Dumping : public IDumping
{

  public:
    Dumping(const Plugin::PluginInterface* pluginInterface, std::shared_ptr<IConfig> configuration);

    ~Dumping() override = default;

    void dumpMemoryRegion(const std::string& processName,
                          pid_t pid,
                          const Plugin::MemoryRegion& memoryRegionDescriptor,
                          const std::vector<uint8_t>& data) override;

    std::vector<std::string> getAllMemoryRegionInformation() override;

  private:
    const Plugin::PluginInterface* pluginInterface;
    std::shared_ptr<IConfig> configuration;

    std::filesystem::path dumpingPath;
    std::filesystem::path inMemoryDumpingFolder;
    std::mutex lock{};
    std::vector<std::string> memoryRegionInfo{};
    int memoryRegionCounter{};
    std::mutex counterLock{};

    static std::unique_ptr<std::string> protectionToString(ProtectionValues protection);

    static std::unique_ptr<MemoryRegionInformation> createMemoryRegionInformation(
        const std::string& processName, pid_t pid, const Plugin::MemoryRegion& memoryRegionDescriptor, int regionId);

    int getNextRegionId();

    void appendRegionInfo(const std::string& regionInfo);
};
