#include "ActiveProcessesSupervisor.h"
#include "../../vmi/VmiException.h"
#include "Constants.h"
#include "MMExtractor.h"
#include <fmt/core.h>
#include <string>
#include <vmicore/filename.h>

namespace VmiCore::Linux
{
    ActiveProcessesSupervisor::ActiveProcessesSupervisor(
        std::shared_ptr<ILibvmiInterface> vmiInterface,
        std::shared_ptr<ILogging> loggingLib, // NOLINT(performance-unnecessary-value-param)
        std::shared_ptr<IEventStream> eventStream)
        : vmiInterface(vmiInterface),
          logging(loggingLib),
          logger(loggingLib->newNamedLogger(FILENAME_STEM)),
          eventStream(std::move(eventStream)),
          pathExtractor(std::move(vmiInterface), loggingLib)
    {
    }

    void ActiveProcessesSupervisor::initialize()
    {
        // Check if kernel is recent enough to have PTI support (backports for LTS releases are currently ignored)
        if (auto [major, minor, _patch] = extractKernelVersion(); major > 4 || (major == 4 && minor >= 15))
        {
            // Check if kernel page table isolation is enabled
            auto x86CapabilityOffset = vmiInterface->getKernelStructOffset("cpuinfo_x86", "x86_capability");
            // X86_FEATURE_PTI is defined as 7*32+11
            auto x86CapabilityEntry = vmiInterface->read32VA(vmiInterface->translateKernelSymbolToVA("boot_cpu_data") +
                                                                 x86CapabilityOffset + PTI_FEATURE_ARRAY_ENTRY_OFFSET,
                                                             vmiInterface->convertPidToDtb(SYSTEM_PID));
            pti = x86CapabilityEntry & PTI_FEATURE_MASK;
        }

        logger->info("--- Initialization ---");
        auto taskOffset = vmiInterface->getOffset("linux_tasks");
        auto initTaskVA = vmiInterface->translateKernelSymbolToVA("init_task") + taskOffset;
        auto currentListEntry = initTaskVA;
        logger->debug("Got VA of initTask", {{"initTaskVA", fmt::format("{:#x}", currentListEntry)}});

        do
        {
            addNewProcess(currentListEntry - taskOffset);
            currentListEntry = vmiInterface->read64VA(currentListEntry, vmiInterface->convertPidToDtb(SYSTEM_PID));
        } while (currentListEntry != initTaskVA);

        logger->info("--- End of Initialization ---");
    }

    std::unique_ptr<ActiveProcessInformation> ActiveProcessesSupervisor::extractProcessInformation(uint64_t taskStruct)
    {
        auto processInformation = std::make_unique<ActiveProcessInformation>();
        processInformation->base = taskStruct;

        auto mm = vmiInterface->read64VA(taskStruct + vmiInterface->getKernelStructOffset("task_struct", "mm"),
                                         vmiInterface->convertPidToDtb(SYSTEM_PID));
        if (mm != 0)
        {
            processInformation->processDtb =
                vmiInterface->convertVAToPA(vmiInterface->read64VA(mm + vmiInterface->getOffset("linux_pgd"),
                                                                   vmiInterface->convertPidToDtb(SYSTEM_PID)),
                                            vmiInterface->convertPidToDtb(SYSTEM_PID));
            processInformation->processUserDtb =
                pti ? processInformation->processDtb + USER_DTB_OFFSET : processInformation->processDtb;
            processInformation->processPath = std::make_unique<std::string>(pathExtractor.extractDPath(
                vmiInterface->read64VA(mm + vmiInterface->getKernelStructOffset("mm_struct", "exe_file"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID)) +
                vmiInterface->getKernelStructOffset("file", "f_path")));
            processInformation->fullName = processInformation->processPath
                                               ? splitProcessFileNameFromPath(*processInformation->processPath)
                                               : nullptr;
            processInformation->memoryRegionExtractor = std::make_unique<MMExtractor>(vmiInterface, logging, mm);
        }

        processInformation->pid = extractPid(taskStruct);
        processInformation->parentPid = vmiInterface->read32VA(
            vmiInterface->read64VA(taskStruct + vmiInterface->getKernelStructOffset("task_struct", "real_parent"),
                                   vmiInterface->convertPidToDtb(SYSTEM_PID)) +
                vmiInterface->getKernelStructOffset("task_struct", "tgid"),
            vmiInterface->convertPidToDtb(SYSTEM_PID));
        processInformation->name = *vmiInterface->extractStringAtVA(taskStruct + vmiInterface->getOffset("linux_name"),
                                                                    vmiInterface->convertPidToDtb(SYSTEM_PID));

        // Special case: The process with pid 0 only consists of idle threads and therefore has got no mm_struct. In
        // this case we simply use the kpgd that's already stored in libvmi.
        if (processInformation->pid == SYSTEM_PID)
        {
            processInformation->processDtb = vmiInterface->convertPidToDtb(SYSTEM_PID);
            processInformation->processUserDtb = processInformation->processDtb;
        }

        return processInformation;
    }

    pid_t ActiveProcessesSupervisor::extractPid(uint64_t taskStruct) const
    {
        return static_cast<pid_t>(vmiInterface->read32VA(taskStruct + vmiInterface->getOffset("linux_pid"),
                                                         vmiInterface->convertPidToDtb(SYSTEM_PID)));
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
    ActiveProcessesSupervisor::getProcessInformationByBase(uint64_t taskStruct) const
    {
        const auto pidIterator = pidsByTaskStruct.find(taskStruct);
        if (pidIterator == pidsByTaskStruct.cend())
        {
            throw std::invalid_argument(
                fmt::format("{}: Process with taskStruct {:#x} not in process cache.", __func__, taskStruct));
        }
        return getProcessInformationByPid(pidIterator->second);
    }

    void ActiveProcessesSupervisor::addNewProcess(uint64_t taskStruct)
    {
        std::shared_ptr<ActiveProcessInformation> processInformation(extractProcessInformation(taskStruct));
        std::string parentPid("unknownParentPid");
        std::string parentName("unknownParentName");
        std::string parentDtb("unknownParentDtb");
        auto parentProcessInformation = processInformationByPid.find(processInformation->parentPid);
        if (parentProcessInformation != processInformationByPid.end())
        {
            parentPid = std::to_string(parentProcessInformation->second->pid);
            parentName = parentProcessInformation->second->name;
            parentDtb = fmt::format("{:#x}", parentProcessInformation->second->processDtb);
        }
        eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                      processInformation->name,
                                      static_cast<uint32_t>(processInformation->pid),
                                      fmt::format("{:#x}", processInformation->processDtb));
        logger->info("Discovered active process",
                     {{"ProcessName", processInformation->name},
                      {"ProcessId", static_cast<uint64_t>(processInformation->pid)},
                      {"ProcessDtb", fmt::format("{:#x}", processInformation->processDtb)},
                      {"ProcessUserDtb", fmt::format("{:#x}", processInformation->processUserDtb)},
                      {"ParentProcessName", parentName},
                      {"ParentProcessId", parentPid},
                      {"ParentProcessDtb", parentDtb}});
        processInformationByPid[processInformation->pid] = processInformation;
        pidsByTaskStruct[processInformation->base] = processInformation->pid;
    }

    void ActiveProcessesSupervisor::removeActiveProcess(uint64_t taskStruct)
    {
        auto taskStructIterator = pidsByTaskStruct.find(taskStruct);
        if (taskStructIterator != pidsByTaskStruct.end())
        {
            auto processInformationIterator = processInformationByPid.find(taskStructIterator->second);
            if (processInformationIterator == processInformationByPid.end())
            {
                logger->warning("Process information not found for process",
                                {{"taskStruct", fmt::format("{:#x}", taskStruct)},
                                 {"ProcessId", static_cast<uint64_t>(taskStructIterator->second)}});
            }
            else
            {
                std::string parentPid("unknownParentPid");
                std::string parentName("unknownParentName");
                std::string parentDtb("unknownParentDtb");
                auto parentProcessInformation =
                    processInformationByPid.find(processInformationIterator->second->parentPid);
                if (parentProcessInformation != processInformationByPid.end())
                {
                    parentPid = std::to_string(parentProcessInformation->second->pid);
                    parentName = parentProcessInformation->second->name;
                    parentDtb = fmt::format("{:#x}", parentProcessInformation->second->processDtb);
                }

                eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                              processInformationIterator->second->name,
                                              static_cast<uint32_t>(processInformationIterator->second->pid),
                                              fmt::format("{:#x}", processInformationIterator->second->processDtb));
                logger->info(
                    "Remove process from actives processes",
                    {{"ProcessName", processInformationIterator->second->name},
                     {"ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)},
                     {"ProcessDtb", fmt::format("{:#x}", processInformationIterator->second->processDtb)},
                     {"ProcessUserDtb", fmt::format("{:#x}", processInformationIterator->second->processUserDtb)},
                     {"ParentProcessName", parentName},
                     {"ParentProcessId", parentPid},
                     {"ParentProcessDtb", parentDtb}});

                processInformationByPid.erase(processInformationIterator);
            }
            pidsByTaskStruct.erase(taskStructIterator);
        }
        else
        {
            logger->warning("Process does not seem to be stored as an active process",
                            {{"taskStruct", fmt::format("{:#x}", taskStruct)}});
        }
    }

    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
    ActiveProcessesSupervisor::getActiveProcesses() const
    {
        auto runningProcesses = std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>();
        for (const auto& element : processInformationByPid)
        {
            runningProcesses->push_back(element.second);
        }
        return runningProcesses;
    }

    std::unique_ptr<std::string> ActiveProcessesSupervisor::splitProcessFileNameFromPath(const std::string& path) const
    {
        auto substringStartIterator =
            std::find_if(path.crbegin(), path.crend(), [](const char c) { return c == '/'; }).base();
        if (substringStartIterator == path.cbegin())
        {
            throw VmiException(fmt::format("{}: Unable to find any path separators at all", __func__));
        }
        return std::make_unique<std::string>(substringStartIterator, path.cend());
    }

    std::tuple<int, int, int> ActiveProcessesSupervisor::extractKernelVersion() const
    {
        auto banner = vmiInterface->extractStringAtVA(vmiInterface->translateKernelSymbolToVA("linux_banner"),
                                                      vmiInterface->convertPidToDtb(SYSTEM_PID));
        logger->debug("Banner extracted", {{"Banner", *banner}});

        std::smatch matches;
        if (!std::regex_search(*banner, matches, kernelBannerVersionMatcher))
        {
            throw std::runtime_error("Unexpected content in kernel banner");
        }
        if (matches.size() < 4)
        {
            throw std::runtime_error("Unable to retrieve version from kernel banner");
        }

        return {std::stoi(matches[1].str()), std::stoi(matches[2].str()), std::stoi(matches[3].str())};
    }
}
