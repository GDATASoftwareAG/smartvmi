#include "ActiveProcessesSupervisor.h"
#include "../../vmi/VmiException.h"
#include <fmt/core.h>
#include <string>
#include <vmicore/filename.h>

namespace VmiCore::Windows
{
    namespace
    {
        constexpr uint64_t statusPending = 0x103;
    }

    ActiveProcessesSupervisor::ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                         std::shared_ptr<IKernelAccess> kernelAccess,
                                                         std::shared_ptr<ILogging> logging,
                                                         std::shared_ptr<IEventStream> eventStream)
        : vmiInterface(std::move(vmiInterface)),
          kernelAccess(std::move(kernelAccess)),
          logger(logging->newNamedLogger(FILENAME_STEM)),
          logging(std::move(logging)),
          eventStream(std::move(eventStream))
    {
    }

    void ActiveProcessesSupervisor::initialize()
    {
        logger->info("--- Initialization ---");
        kernelAccess->initWindowsOffsets();
        auto psActiveProcessListHeadVA = vmiInterface->translateKernelSymbolToVA("PsActiveProcessHead");
        logger->debug("Got VA of PsActiveProcessHead",
                      {{"PsActiveProcessHeadVA", fmt::format("{:#x}", psActiveProcessListHeadVA)}});

        auto currentListEntry =
            vmiInterface->read64VA(psActiveProcessListHeadVA, vmiInterface->convertPidToDtb(SYSTEM_PID));
        while (currentListEntry != psActiveProcessListHeadVA)
        {
            addNewProcess(kernelAccess->getCurrentProcessEprocessBase(currentListEntry));
            currentListEntry = vmiInterface->read64VA(currentListEntry, vmiInterface->convertPidToDtb(SYSTEM_PID));
        }

        logger->info("--- End of Initialization ---");
    }

    std::unique_ptr<ActiveProcessInformation>
    ActiveProcessesSupervisor::extractProcessInformation(uint64_t eprocessBase) const
    {
        auto processInformation = std::make_unique<ActiveProcessInformation>();
        processInformation->base = eprocessBase;
        processInformation->processDtb = kernelAccess->extractDirectoryTableBase(eprocessBase);
        processInformation->processUserDtb = kernelAccess->extractUserDirectoryTableBase(eprocessBase);
        // KPTI implemented but inactive
        if (processInformation->processUserDtb == 0)
        {
            processInformation->processUserDtb = processInformation->processDtb;
        }
        processInformation->pid = kernelAccess->extractPID(eprocessBase);
        processInformation->parentPid = kernelAccess->extractParentID(eprocessBase);
        processInformation->name = kernelAccess->extractImageFileName(eprocessBase);
        processInformation->is32BitProcess = kernelAccess->extractIsWow64Process(eprocessBase);
        try
        {
            processInformation->processPath = extractProcessPath(eprocessBase);
            processInformation->fullName = splitProcessFileNameFromPath(*processInformation->processPath);
        }
        catch (const std::exception& e)
        {
            processInformation->processPath = std::make_unique<std::string>();
            processInformation->fullName = std::make_unique<std::string>();
            logger->warning("Process",
                            {{"ProcessName", processInformation->name},
                             {"ProcessId", static_cast<uint64_t>(processInformation->pid)},
                             {"Exception", e.what()}});
        }
        processInformation->memoryRegionExtractor = std::make_unique<VadTreeWin10>(
            kernelAccess, eprocessBase, processInformation->pid, processInformation->name, logging);

        return processInformation;
    }

    std::shared_ptr<ActiveProcessInformation> ActiveProcessesSupervisor::getSystemProcessInformation() const
    {
        return getProcessInformationByPid(SYSTEM_PID);
    }

    std::shared_ptr<ActiveProcessInformation> ActiveProcessesSupervisor::getProcessInformationByPid(pid_t pid) const
    {
        std::shared_ptr<ActiveProcessInformation> processInformation;
        try
        {
            processInformation = processInformationByPid.at(pid);
        }
        catch (const std::out_of_range&)
        {
            throw std::invalid_argument("Unable to find process with pid " + std::to_string(pid));
        }
        return processInformation;
    }

    std::shared_ptr<ActiveProcessInformation>
    ActiveProcessesSupervisor::getProcessInformationByBase(uint64_t eprocessBase) const
    {
        const auto pidIterator = pidsByEprocessBase.find(eprocessBase);
        if (pidIterator == pidsByEprocessBase.cend())
        {
            throw std::invalid_argument(
                fmt::format("{}: Process with _EPROCESS base {:#x} not in process cache.", __func__, eprocessBase));
        }
        return getProcessInformationByPid(pidIterator->second);
    }

    void ActiveProcessesSupervisor::addNewProcess(uint64_t eprocessBase)
    {
        std::shared_ptr<ActiveProcessInformation> processInformation(extractProcessInformation(eprocessBase));
        std::string parentPid("unknownParentPid");
        std::string parentName("unknownParentName");
        std::string parentCr3("unknownParentCr3");
        auto parentProcessInformation = processInformationByPid.find(processInformation->parentPid);
        if (parentProcessInformation != processInformationByPid.end())
        {
            parentPid = std::to_string(parentProcessInformation->second->pid);
            parentName = parentProcessInformation->second->name;
            parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->processDtb);
        }
        eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                      processInformation->name,
                                      static_cast<uint32_t>(processInformation->pid),
                                      fmt::format("{:#x}", processInformation->processDtb));
        logger->info("Discovered active process",
                     {{"ProcessName", processInformation->name},
                      {"ProcessId", static_cast<uint64_t>(processInformation->pid)},
                      {"ProcessCr3", fmt::format("{:#x}", processInformation->processDtb)},
                      {"ParentProcessName", parentName},
                      {"ParentProcessId", parentPid},
                      {"ParentProcessCr3", parentCr3}});
        processInformationByPid[processInformation->pid] = processInformation;
        pidsByEprocessBase[processInformation->base] = processInformation->pid;
    }

    bool ActiveProcessesSupervisor::isProcessActive(uint64_t eprocessBase) const
    {
        auto exitStatus = kernelAccess->extractExitStatus(eprocessBase);
        if (exitStatus == statusPending)
        {
            return true;
        }
        logger->debug("Encountered a process that has got an exit status other than 'status pending'",
                      {{"_EPROCESS_base", fmt::format("{:#x}", eprocessBase)},
                       {"ProcessId", static_cast<uint64_t>(kernelAccess->extractPID(eprocessBase))},
                       CxxLogField("ExitStatus", static_cast<uint64_t>(exitStatus))

                      });
        return false;
    }

    void ActiveProcessesSupervisor::removeActiveProcess(uint64_t eprocessBase)
    {
        auto eprocessBaseIterator = pidsByEprocessBase.find(eprocessBase);
        if (eprocessBaseIterator != pidsByEprocessBase.end())
        {
            auto processInformationIterator = processInformationByPid.find(eprocessBaseIterator->second);
            if (processInformationIterator == processInformationByPid.end())
            {
                logger->warning("Process information not found for process",
                                {{"_EPROCESS_base", fmt::format("{:#x}", eprocessBase)},
                                 CxxLogField("ProcessId", static_cast<uint64_t>(eprocessBaseIterator->second))

                                });
            }
            else
            {
                std::string parentPid("unknownParentPid");
                std::string parentName("unknownParentName");
                std::string parentCr3("unknownParentCr3");
                auto parentProcessInformation =
                    processInformationByPid.find(processInformationIterator->second->parentPid);
                if (parentProcessInformation != processInformationByPid.end())
                {
                    parentPid = std::to_string(parentProcessInformation->second->pid);
                    parentName = parentProcessInformation->second->name;
                    parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->processDtb);
                }

                eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                              processInformationIterator->second->name,
                                              static_cast<uint32_t>(processInformationIterator->second->pid),
                                              fmt::format("{:#x}", processInformationIterator->second->processDtb));
                logger->info(
                    "Remove process from actives processes",
                    {{"ProcessName", processInformationIterator->second->name},
                     {"ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)},
                     CxxLogField("ProcessCr3", fmt::format("{:#x}", processInformationIterator->second->processDtb)),
                     {"ParentProcessName", parentName},
                     {"ParentProcessId", parentPid},
                     {"ParentProcessCr3", parentCr3}});

                processInformationByPid.erase(processInformationIterator);
            }
            pidsByEprocessBase.erase(eprocessBaseIterator);
        }
        else
        {
            logger->warning("Process does not seem to be stored as an active process",
                            {{"_EPROCESS_base", fmt::format("{:#x}", eprocessBase)}});
        }
    }

    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
    ActiveProcessesSupervisor::getActiveProcesses() const
    {
        auto runningProcesses = std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>();
        for (const auto& element : processInformationByPid)
        {
            if (isProcessActive(element.second->base))
            {
                runningProcesses->push_back(element.second);
            }
        }
        return runningProcesses;
    }

    std::unique_ptr<std::string> ActiveProcessesSupervisor::extractProcessPath(uint64_t eprocessBase) const
    {
        auto sectionAddress = kernelAccess->extractSectionAddress(eprocessBase);
        auto controlAreaAddress = kernelAccess->extractControlAreaAddress(sectionAddress);
        auto fileFlag = kernelAccess->extractIsFile(controlAreaAddress);
        if (!fileFlag)
        {
            throw VmiException(fmt::format("{}: File flag in mmSectionFlags not set", __func__));
        }
        auto controlAreaFilePointer = kernelAccess->extractControlAreaFilePointer(controlAreaAddress);
        auto filePointerAddress = KernelAccess::removeReferenceCountFromExFastRef(controlAreaFilePointer);
        auto processPath = kernelAccess->extractProcessPath(filePointerAddress);

        return processPath;
    }

    std::unique_ptr<std::string> ActiveProcessesSupervisor::splitProcessFileNameFromPath(const std::string& path)
    {
        auto substringStartIterator =
            std::find_if(path.crbegin(), path.crend(), [](const char c) { return c == '\\'; }).base();
        if (substringStartIterator == path.cbegin())
        {
            throw VmiException(fmt::format("{}: Unable to find any path separators at all", __func__));
        }
        return std::make_unique<std::string>(substringStartIterator, path.cend());
    }
}
