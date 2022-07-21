#include "ActiveProcessesSupervisor.h"
#include <fmt/core.h>

namespace
{
    constexpr uint64_t statusPending = 0x103;
}

ActiveProcessesSupervisor::ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                     std::shared_ptr<IKernelAccess> kernelAccess,
                                                     std::shared_ptr<ILogging> loggingLib,
                                                     std::shared_ptr<IEventStream> eventStream)
    : vmiInterface(std::move(vmiInterface)),
      kernelAccess(std::move(kernelAccess)),
      logger(NEW_LOGGER(loggingLib)),
      loggingLib(std::move(loggingLib)),
      eventStream(std::move(eventStream))
{
}

void ActiveProcessesSupervisor::initialize()
{
    logger->info("--- Initialization ---");
    kernelAccess->initWindowsOffsets();
    auto psActiveProcessListHeadVA = vmiInterface->translateKernelSymbolToVA("PsActiveProcessHead");
    auto currentListEntry = psActiveProcessListHeadVA;
    logger->debug("Got VA of PsActiveProcessHead",
                  {logfield::create("PsActiveProcessHeadVA", fmt::format("{:#x}", currentListEntry))});

    do
    {
        addNewProcess(kernelAccess->getCurrentProcessEprocessBase(currentListEntry));
        currentListEntry = vmiInterface->read64VA(currentListEntry, vmiInterface->getSystemCr3());
    } while (currentListEntry != psActiveProcessListHeadVA);
    logger->info("--- End of Initialization ---");
}

std::unique_ptr<ActiveProcessInformation>
ActiveProcessesSupervisor::extractProcessInformation(uint64_t eprocessBase) const
{
    auto processInformation = std::make_unique<ActiveProcessInformation>();
    processInformation->eprocessBase = eprocessBase;
    processInformation->processCR3 = kernelAccess->extractDirectoryTableBase(eprocessBase);
    processInformation->pid = kernelAccess->extractPID(eprocessBase);
    processInformation->parentPid = kernelAccess->extractParentID(eprocessBase);
    processInformation->name = kernelAccess->extractImageFileName(eprocessBase);
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
                        {logfield::create("ProcessName", processInformation->name),
                         logfield::create("ProcessId", static_cast<uint64_t>(processInformation->pid)),
                         logfield::create("Exception", e.what())});
    }
    processInformation->vadTree = std::make_unique<VadTreeWin10>(
        kernelAccess, eprocessBase, processInformation->pid, processInformation->name, loggingLib);
    return processInformation;
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
ActiveProcessesSupervisor::getProcessInformationByEprocessBase(uint64_t eprocessBase)
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
        parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->processCR3);
    }
    eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                  processInformation->name,
                                  static_cast<uint32_t>(processInformation->pid),
                                  fmt::format("{:#x}", processInformation->processCR3));
    logger->info("Discovered active process",
                 {logfield::create("ProcessName", processInformation->name),
                  logfield::create("ProcessId", static_cast<uint64_t>(processInformation->pid)),
                  logfield::create("ProcessCr3", fmt::format("{:#x}", processInformation->processCR3)),
                  logfield::create("ParentProcessName", parentName),
                  logfield::create("ParentProcessId", parentPid),
                  logfield::create("ParentProcessCr3", parentCr3)});
    processInformationByPid[processInformation->pid] = processInformation;
    pidsByEprocessBase[processInformation->eprocessBase] = processInformation->pid;
}

bool ActiveProcessesSupervisor::isProcessActive(uint64_t eprocessBase) const
{
    auto exitStatus = kernelAccess->extractExitStatus(eprocessBase);
    if (exitStatus == statusPending)
    {
        return true;
    }
    logger->debug("Encountered a process that has got an exit status other than 'status pending'",
                  {logfield::create("_EPROCESS_base", fmt::format("{:#x}", eprocessBase)),
                   logfield::create("ProcessId", static_cast<uint64_t>(kernelAccess->extractPID(eprocessBase))),
                   logfield::create("ExitStatus", static_cast<uint64_t>(exitStatus))

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
                            {logfield::create("_EPROCESS_base", fmt::format("{:#x}", eprocessBase)),
                             logfield::create("ProcessId", static_cast<uint64_t>(eprocessBaseIterator->second))

                            });
        }
        else
        {
            std::string parentPid("unknownParentPid");
            std::string parentName("unknownParentName");
            std::string parentCr3("unknownParentCr3");
            auto parentProcessInformation = processInformationByPid.find(processInformationIterator->second->parentPid);
            if (parentProcessInformation != processInformationByPid.end())
            {
                parentPid = std::to_string(parentProcessInformation->second->pid);
                parentName = parentProcessInformation->second->name;
                parentCr3 = fmt::format("{:#x}", parentProcessInformation->second->processCR3);
            }

            eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                          processInformationIterator->second->name,
                                          static_cast<uint32_t>(processInformationIterator->second->pid),
                                          fmt::format("{:#x}", processInformationIterator->second->processCR3));
            logger->info(
                "Remove process from actives processes",
                {logfield::create("ProcessName", processInformationIterator->second->name),
                 logfield::create("ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)),
                 logfield::create("ProcessCr3", fmt::format("{:#x}", processInformationIterator->second->processCR3)),
                 logfield::create("ParentProcessName", parentName),
                 logfield::create("ParentProcessId", parentPid),
                 logfield::create("ParentProcessCr3", parentCr3)});

            processInformationByPid.erase(processInformationIterator);
        }
        pidsByEprocessBase.erase(eprocessBaseIterator);
    }
    else
    {
        logger->warning("Process does not seem to be stored as an active process",
                        {logfield::create("_EPROCESS_base", fmt::format("{:#x}", eprocessBase))});
    }
}

std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> ActiveProcessesSupervisor::getActiveProcesses()
{
    auto runningProcesses = std::make_unique<std::vector<std::shared_ptr<ActiveProcessInformation>>>();
    for (const auto& element : processInformationByPid)
    {
        if (isProcessActive(element.second->eprocessBase))
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
