#include "ActiveProcessesSupervisor.h"
#include "../../Convenience.h"

namespace
{
    constexpr uint64_t statusPending = 0x103;
}

ActiveProcessesSupervisor::ActiveProcessesSupervisor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                                     std::shared_ptr<IKernelObjectExtractorWin10> kernelObjectExtractor,
                                                     std::shared_ptr<ILogging> loggingLib,
                                                     std::shared_ptr<IEventStream> eventStream)
    : vmiInterface(std::move(vmiInterface)),
      kernelObjectExtractor(std::move(kernelObjectExtractor)),
      logger(NEW_LOGGER(loggingLib)),
      loggingLib(std::move(loggingLib)),
      eventStream(std::move(eventStream))
{
}

void ActiveProcessesSupervisor::initialize()
{
    logger->info("--- Initialization ---");
    auto psActiveProcessListHeadVA = vmiInterface->translateKernelSymbolToVA("PsActiveProcessHead");
    auto currentListEntry = psActiveProcessListHeadVA;
    logger->debug("Got VA of PsActiveProcessHead",
                  {logfield::create("PsActiveProcessHeadVA", Convenience::intToHex(currentListEntry))});

    do
    {
        auto currentProcessEprocessBase = currentListEntry - OffsetDefinitionsWin10::_EPROCESS::ActiveProcessLinks;
        addNewProcess(currentProcessEprocessBase);
        currentListEntry = vmiInterface->read64VA(currentListEntry, vmiInterface->getSystemCr3());
    } while (currentListEntry != psActiveProcessListHeadVA);
    logger->info("--- End of Initialization ---");
}

std::unique_ptr<ActiveProcessInformation>
ActiveProcessesSupervisor::extractProcessInformation(uint64_t eprocessBase) const
{
    auto processInformation = std::make_unique<ActiveProcessInformation>();
    processInformation->eprocessBase = eprocessBase;
    processInformation->processCR3 = vmiInterface->read64VA(
        eprocessBase + OffsetDefinitionsWin10::_KPROCESS::DirectoryTableBase, vmiInterface->getSystemCr3());
    processInformation->pid = extractPid(eprocessBase);
    processInformation->parentPid = vmiInterface->read32VA(
        eprocessBase + OffsetDefinitionsWin10::_EPROCESS::InheritedFromUniqueProcessId, vmiInterface->getSystemCr3());
    processInformation->name = *vmiInterface->extractStringAtVA(
        eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ImageFileName, vmiInterface->getSystemCr3());
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
        kernelObjectExtractor, eprocessBase, processInformation->pid, processInformation->name, loggingLib);
    return processInformation;
}

pid_t ActiveProcessesSupervisor::extractPid(uint64_t eprocessBase) const
{
    return vmiInterface->read32VA(eprocessBase + OffsetDefinitionsWin10::_EPROCESS::UniqueProcessId,
                                  vmiInterface->getSystemCr3());
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
        throw std::invalid_argument(std::string(__func__) + ": Process with _EPROCESS base " +
                                    Convenience::intToHex(eprocessBase) + " not in process cache.");
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
        parentCr3 = Convenience::intToHex(parentProcessInformation->second->processCR3);
    }
    eventStream->sendProcessEvent(::grpc::ProcessState::Started,
                                  processInformation->name,
                                  static_cast<uint32_t>(processInformation->pid),
                                  Convenience::intToHex(processInformation->processCR3));
    logger->info("Discovered active process",
                 {logfield::create("ProcessName", processInformation->name),
                  logfield::create("ProcessId", static_cast<uint64_t>(processInformation->pid)),
                  logfield::create("ProcessCr3", Convenience::intToHex(processInformation->processCR3)),
                  logfield::create("ParentProcessName", parentName),
                  logfield::create("ParentProcessId", parentPid),
                  logfield::create("ParentProcessCr3", parentCr3)});
    processInformationByPid[processInformation->pid] = processInformation;
    pidsByEprocessBase[processInformation->eprocessBase] = processInformation->pid;
}

bool ActiveProcessesSupervisor::isProcessActive(uint64_t eprocessBase) const
{
    auto exitStatus = vmiInterface->read32VA(eprocessBase + OffsetDefinitionsWin10::_EPROCESS::ExitStatus,
                                             vmiInterface->getSystemCr3());
    if (exitStatus == statusPending)
    {
        return true;
    }
    logger->debug("Encountered a process that has got an exit status other than 'status pending'",
                  {logfield::create("_EPROCESS_base", Convenience::intToHex(eprocessBase)),
                   logfield::create("ProcessId", static_cast<uint64_t>(extractPid(eprocessBase))),
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
                            {logfield::create("_EPROCESS_base", Convenience::intToHex(eprocessBase)),
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
                parentCr3 = Convenience::intToHex(parentProcessInformation->second->processCR3);
            }

            eventStream->sendProcessEvent(::grpc::ProcessState::Terminated,
                                          processInformationIterator->second->name,
                                          static_cast<uint32_t>(processInformationIterator->second->pid),
                                          Convenience::intToHex(processInformationIterator->second->processCR3));
            logger->info(
                "Remove process from actives processes",
                {logfield::create("ProcessName", processInformationIterator->second->name),
                 logfield::create("ProcessId", static_cast<uint64_t>(processInformationIterator->second->pid)),
                 logfield::create("ProcessCr3", Convenience::intToHex(processInformationIterator->second->processCR3)),
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
                        {logfield::create("_EPROCESS_base", Convenience::intToHex(eprocessBase))});
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

// is duplicate of same function in VadTreeWin10.cpp, do not see a nice solution to this at the moment
uint64_t ActiveProcessesSupervisor::removeReferenceCountFromExFastRef(uint64_t exFastRefValue)
{
    return exFastRefValue & ~(static_cast<uint64_t>(0xF));
}

std::unique_ptr<std::string> ActiveProcessesSupervisor::extractProcessPath(uint64_t eprocessBase) const
{
    auto processPath = std::make_unique<std::string>();
    auto sectionAddress = vmiInterface->read64VA(eprocessBase + OffsetDefinitionsWin10::_EPROCESS::SectionObject,
                                                 vmiInterface->getSystemCr3());
    if (sectionAddress == 0)
    {
        throw VmiException(std::string(__func__) + ": Section base address is null");
    }

    auto controlAreaAddress =
        vmiInterface->read64VA(sectionAddress + OffsetDefinitionsWin10::_SECTION::u1, vmiInterface->getSystemCr3());
    if (controlAreaAddress == 0)
    {
        throw VmiException(std::string(__func__) + ": Control area base address is null");
    }

    auto sectionFlags =
        kernelObjectExtractor->extractMmSectionFlags(controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::u);
    if (sectionFlags->File == 0)
    {
        throw VmiException(std::string(__func__) + ": File flag in mmSectionFlags not set");
    }

    auto filePointerAddress = removeReferenceCountFromExFastRef(vmiInterface->read64VA(
        controlAreaAddress + OffsetDefinitionsWin10::_CONTROL_AREA::FilePointer, vmiInterface->getSystemCr3()));
    if (filePointerAddress == 0)
    {
        throw VmiException(std::string(__func__) + ": File pointer base address is null");
    }

    auto fileName = kernelObjectExtractor->extractUnicodeObject(filePointerAddress +
                                                                OffsetDefinitionsWin10::_FILE_OBJECT::FileName);
    if (fileName->Buffer == nullptr || fileName->Length == 0)
    {
        throw VmiException(std::string(__func__) + ": Unicode string is not valid");
    }
    processPath = kernelObjectExtractor->extractWString(reinterpret_cast<uint64_t>(fileName->Buffer), fileName->Length);
    return processPath;
}

std::unique_ptr<std::string> ActiveProcessesSupervisor::splitProcessFileNameFromPath(const std::string& path) const
{
    auto substringStartIterator =
        std::find_if(path.crbegin(), path.crend(), [](const char c) { return c == '\\'; }).base();
    if (substringStartIterator == path.cbegin())
    {
        throw VmiException(std::string(__func__) + ": Unable to find any path separators at all");
    }
    return std::make_unique<std::string>(substringStartIterator, path.cend());
}
