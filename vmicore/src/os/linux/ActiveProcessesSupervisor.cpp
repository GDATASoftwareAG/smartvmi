#include "ActiveProcessesSupervisor.h"
#include "ProtectionValues.h"
#include <fmt/core.h>
#include <string>

namespace Linux
{
    ActiveProcessesSupervisor::ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                         std::shared_ptr<ILogging> loggingLib,
                                                         std::shared_ptr<IEventStream> eventStream)
        : vmiInterface(std::move(vmiInterface)),
          logger(NEW_LOGGER(loggingLib)),
          loggingLib(std::move(loggingLib)),
          eventStream(std::move(eventStream))
    {
    }

    void ActiveProcessesSupervisor::initialize()
    {
        logger->info("--- Initialization ---");
        auto taskOffset = vmiInterface->getOffset("linux_tasks");
        auto initTaskVA = vmiInterface->translateKernelSymbolToVA("init_task") + taskOffset;
        auto currentListEntry = initTaskVA;
        logger->debug("Got VA of initTask", {logfield::create("initTaskVA", fmt::format(":#x", currentListEntry))});

        do
        {
            addNewProcess(currentListEntry - taskOffset);
            currentListEntry = vmiInterface->read64VA(currentListEntry, vmiInterface->convertPidToDtb(systemPid));
        } while (currentListEntry != initTaskVA);
        logger->info("--- End of Initialization ---");
    }

    std::unique_ptr<ActiveProcessInformation>
    ActiveProcessesSupervisor::extractProcessInformation(uint64_t taskStruct) const
    {
        auto processInformation = std::make_unique<ActiveProcessInformation>();
        processInformation->base = taskStruct;
        if (auto mm = vmiInterface->read64VA(taskStruct + vmiInterface->getKernelStructOffset("task_struct", "mm"),
                                             vmiInterface->convertPidToDtb(systemPid)))
        {
            processInformation->processCR3 =
                vmiInterface->convertVAToPA(vmiInterface->read64VA(mm + vmiInterface->getOffset("linux_pgd"),
                                                                   vmiInterface->convertPidToDtb(systemPid)),
                                            vmiInterface->convertPidToDtb(systemPid));
            processInformation->processPath =
                extractDPath(vmiInterface->read64VA(mm + vmiInterface->getKernelStructOffset("mm_struct", "exe_file"),
                                                    vmiInterface->convertPidToDtb(systemPid)) +
                             vmiInterface->getKernelStructOffset("file", "f_path"));
            processInformation->fullName = processInformation->processPath
                                               ? splitProcessFileNameFromPath(*processInformation->processPath)
                                               : nullptr;
            processInformation->memoryRegions = std::make_unique<std::vector<MemoryRegion>>();
            extractMemoryRegions(*processInformation->memoryRegions, mm);
        }
        else
        {
            processInformation->processCR3 = 0;
            processInformation->processPath = nullptr;
            processInformation->fullName = nullptr;
            processInformation->memoryRegions = nullptr;
        }

        processInformation->pid = extractPid(taskStruct);
        processInformation->parentPid = vmiInterface->read32VA(
            vmiInterface->read64VA(taskStruct + vmiInterface->getKernelStructOffset("task_struct", "real_parent"),
                                   vmiInterface->convertPidToDtb(systemPid)) +
                vmiInterface->getKernelStructOffset("task_struct", "tgid"),
            vmiInterface->convertPidToDtb(systemPid));
        processInformation->name = *vmiInterface->extractStringAtVA(taskStruct + vmiInterface->getOffset("linux_name"),
                                                                    vmiInterface->convertPidToDtb(systemPid));

        return processInformation;
    }

    void ActiveProcessesSupervisor::extractMemoryRegions(std::vector<MemoryRegion>& regions, uint64_t mm) const
    {
        for (auto area = vmiInterface->read64VA(mm, vmiInterface->convertPidToDtb(systemPid)); area != 0;
             area = vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_next"),
                                           vmiInterface->convertPidToDtb(systemPid)))
        {
            const auto start =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_start"),
                                       vmiInterface->convertPidToDtb(systemPid));
            const auto end =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_end"),
                                       vmiInterface->convertPidToDtb(systemPid));
            const auto size = end - start + 1;
            const auto flags =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_flags"),
                                       vmiInterface->convertPidToDtb(systemPid));
            const auto file =
                vmiInterface->read64VA(area + vmiInterface->getKernelStructOffset("vm_area_struct", "vm_file"),
                                       vmiInterface->convertPidToDtb(systemPid));
            std::string fileName{}, permissions{};
            if (file != 0)
            {
                if (auto path = extractDPath(file + vmiInterface->getKernelStructOffset("file", "f_path")))
                    fileName = *path;
            }

            if (flags & static_cast<uint8_t>(ProtectionValues::VM_READ))
                permissions += "r";

            if (flags & static_cast<uint8_t>(ProtectionValues::VM_WRITE))
                permissions += "w";

            if (flags & static_cast<uint8_t>(ProtectionValues::VM_EXEC))
                permissions += "x";

            logger->debug("Memory Region",
                          {logfield::create("start", fmt::format(":#x", start)),
                           logfield::create("end", fmt::format(":#x", end)),
                           logfield::create("size", size),
                           logfield::create("permissions", permissions),
                           logfield::create("filename", fileName)});
            regions.emplace_back(start,
                                 size,
                                 fileName,
                                 PageProtection(flags, OperatingSystem::LINUX),
                                 !!(flags & static_cast<uint8_t>(ProtectionValues::VM_SHARED)),
                                 false,
                                 false);
        }
    }

    pid_t ActiveProcessesSupervisor::extractPid(uint64_t taskStruct) const
    {
        return vmiInterface->read32VA(taskStruct + vmiInterface->getOffset("linux_pid"),
                                      vmiInterface->convertPidToDtb(systemPid));
    }

    std::shared_ptr<ActiveProcessInformation> ActiveProcessesSupervisor::getProcessInformationByPid(pid_t pid)
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
    ActiveProcessesSupervisor::getProcessInformationByBase(uint64_t taskStruct)
    {
        const auto pidIterator = pidsByTaskStruct.find(taskStruct);
        if (pidIterator == pidsByTaskStruct.cend())
        {
            throw std::invalid_argument(std::string(__func__) + ": Process with taskStruct " +
                                        fmt::format(":#x", taskStruct) + " not in process cache.");
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
            parentCr3 = fmt::format(":#x", parentProcessInformation->second->processCR3);
        }
        eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                      processInformation->name,
                                      static_cast<uint32_t>(processInformation->pid),
                                      fmt::format(":#x", processInformation->processCR3));
        logger->info("Discovered active process",
                     {logfield::create("ProcessName", processInformation->name),
                      logfield::create("ProcessId", static_cast<uint64_t>(processInformation->pid)),
                      logfield::create("ProcessCr3", fmt::format(":#x", processInformation->processCR3)),
                      logfield::create("ParentProcessName", parentName),
                      logfield::create("ParentProcessId", parentPid),
                      logfield::create("ParentProcessCr3", parentCr3)});
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
                                {logfield::create("taskStruct", fmt::format(":#x", taskStruct)),
                                 logfield::create("ProcessId", static_cast<uint64_t>(taskStructIterator->second))

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
                    parentCr3 = fmt::format(":#x", parentProcessInformation->second->processCR3);
                }

                eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                              processInformationIterator->second->name,
                                              static_cast<uint32_t>(processInformationIterator->second->pid),
                                              fmt::format(":#x", processInformationIterator->second->processCR3));
                logger->info(
                    "Remove process from actives processes",
                    {logfield::create("ProcessName", processInformationIterator->second->name),
                     logfield::create("ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)),
                     logfield::create("ProcessCr3", fmt::format(":#x", processInformationIterator->second->processCR3)),
                     logfield::create("ParentProcessName", parentName),
                     logfield::create("ParentProcessId", parentPid),
                     logfield::create("ParentProcessCr3", parentCr3)});

                processInformationByPid.erase(processInformationIterator);
            }
            pidsByTaskStruct.erase(taskStructIterator);
        }
        else
        {
            logger->warning("Process does not seem to be stored as an active process",
                            {logfield::create("taskStruct", fmt::format(":#x", taskStruct))});
        }
    }

    std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
    ActiveProcessesSupervisor::getActiveProcesses()
    {
        auto runningProcesses = std::make_unique<std::vector<std::shared_ptr<const ActiveProcessInformation>>>();
        for (const auto& element : processInformationByPid)
        {
            runningProcesses->push_back(element.second);
        }
        return runningProcesses;
    }

    std::unique_ptr<std::string> ActiveProcessesSupervisor::extractDPath(uint64_t path) const
    {
        std::array<char, PATH_MAX_LINUX> buf{0};

        if (path == 0)
            return nullptr;

        const auto mnt = vmiInterface->read64VA(path + vmiInterface->getKernelStructOffset("path", "mnt"),
                                                vmiInterface->convertPidToDtb(systemPid));
        const auto dentry = vmiInterface->read64VA(path + vmiInterface->getKernelStructOffset("path", "dentry"),
                                                   vmiInterface->convertPidToDtb(systemPid));

        if (dentry == 0 || mnt == 0)
            return nullptr;

        createPath(dentry, mnt - vmiInterface->getKernelStructOffset("mount", "mnt"), buf.data());
        return std::make_unique<std::string>(buf.data());
    }

    void ActiveProcessesSupervisor::createPath(uint64_t dentry, uint64_t mnt, char* buf) const
    {
        try
        {
            const auto name = vmiInterface->extractStringAtVA(
                vmiInterface->read64VA(dentry + vmiInterface->getKernelStructOffset("dentry", "d_name") +
                                           vmiInterface->getKernelStructOffset("qstr", "name"),
                                       vmiInterface->convertPidToDtb(systemPid)),
                vmiInterface->convertPidToDtb(systemPid));
            const auto parent =
                vmiInterface->read64VA(dentry + vmiInterface->getKernelStructOffset("dentry", "d_parent"),
                                       vmiInterface->convertPidToDtb(systemPid));
            const auto mntRoot = vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt"),
                                                        vmiInterface->convertPidToDtb(systemPid));
            const auto mntMountpoint =
                vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt_mountpoint"),
                                       vmiInterface->convertPidToDtb(systemPid));
            const auto mntParent =
                vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt_parent"),
                                       vmiInterface->convertPidToDtb(systemPid));

            if (parent != dentry && dentry != mntRoot)
                createPath(parent, mnt, buf);
            else if (mntParent != mnt)
                createPath(mntMountpoint, mntParent, buf);

            if (parent == dentry && name->at(0) == '/')
            {
                // nothing at all.
            }
            else if (dentry == mntRoot)
            {
                // root (vfs-local or global).
            }
            else if (parent == dentry)
                strcat(buf, name->data());
            else
            {
                strcat(buf, "/");
                strcat(buf, name->data());
            }
        }
        catch (std::exception& e)
        {
            memset(buf, 0, PATH_MAX_LINUX);
        }
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
