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
            processInformation->kernelProcessDTB =
                vmiInterface->convertVAToPA(vmiInterface->read64VA(mm + vmiInterface->getOffset("linux_pgd"),
                                                                   vmiInterface->convertPidToDtb(SYSTEM_PID)),
                                            vmiInterface->convertPidToDtb(SYSTEM_PID));
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
        std::string parentCr3("unknownParentCr3");
        auto parentProcessInformation = processInformationByPid.find(processInformation->parentPid);
        if (parentProcessInformation != processInformationByPid.end())
        {
            parentPid = std::to_string(parentProcessInformation->second->pid);
            parentName = parentProcessInformation->second->name;
            parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->kernelProcessDTB);
        }
        eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                      processInformation->name,
                                      static_cast<uint32_t>(processInformation->pid),
                                      fmt::format("{:#x}", processInformation->kernelProcessDTB));
        logger->info("Discovered active process",
                     {{"ProcessName", processInformation->name},
                      {"ProcessId", static_cast<uint64_t>(processInformation->pid)},
                      {"ProcessCr3", fmt::format("{:#x}", processInformation->kernelProcessDTB)},
                      {"ParentProcessName", parentName},
                      {"ParentProcessId", parentPid},
                      {"ParentProcessCr3", parentCr3}});
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
                std::string parentCr3("unknownParentCr3");
                auto parentProcessInformation =
                    processInformationByPid.find(processInformationIterator->second->parentPid);
                if (parentProcessInformation != processInformationByPid.end())
                {
                    parentPid = std::to_string(parentProcessInformation->second->pid);
                    parentName = parentProcessInformation->second->name;
                    parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->kernelProcessDTB);
                }

                eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                              processInformationIterator->second->name,
                                              static_cast<uint32_t>(processInformationIterator->second->pid),
                                              fmt::format("{:#x}", processInformationIterator->second->kernelProcessDTB));
                logger->info(
                    "Remove process from actives processes",
                    {{"ProcessName", processInformationIterator->second->name},
                     {"ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)},
                     CxxLogField("ProcessCr3", fmt::format("{:#x}", processInformationIterator->second->kernelProcessDTB)),
                     {"ParentProcessName", parentName},
                     {"ParentProcessId", parentPid},
                     {"ParentProcessCr3", parentCr3}});

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
}
