#include "../../vmi/ProcessesMemoryState.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fmt/core.h>

using testing::Contains;
using testing::Not;
using testing::StrEq;
using testing::UnorderedElementsAre;

class ActiveProcessesSupervisorFixture : public ProcessesMemoryStateFixture
{
    void SetUp() override
    {
        ProcessesMemoryStateFixture::SetUp();
        ProcessesMemoryStateFixture::setupActiveProcesses();
    }
};

MATCHER_P(IsEqualProcess, expectedProcess, "")
{
    bool isEqual = false;
    if (arg)
    {
        auto eprocessBasesMatch = expectedProcess.eprocessBase == arg->eprocessBase;
        auto cr3sMatch = expectedProcess.directoryTableBase == arg->processCR3;
        auto namesMatch = expectedProcess.imageFileName == arg->name;
        auto fullNamesMatch = expectedProcess.fullName == *arg->fullName;
        auto pidsMatch = expectedProcess.processId == arg->pid;

        if (eprocessBasesMatch && cr3sMatch && namesMatch && fullNamesMatch && pidsMatch)
        {
            isEqual = true;
        }
    }
    return isEqual;
}

TEST_F(ActiveProcessesSupervisorFixture, initialize_preexistingProcesses_processEventsGenerated)
{
    EXPECT_CALL(*mockEventStream,
                sendProcessEvent(::grpc::ProcessState::Started,
                                 StrEq(emptyFileName),
                                 process0.processId,
                                 StrEq(fmt::format("{:x}", process0.cr3))))
        .Times(1);
    EXPECT_CALL(*mockEventStream,
                sendProcessEvent(::grpc::ProcessState::Started,
                                 StrEq(process4.imageFileName),
                                 process4.processId,
                                 StrEq(fmt::format("{:x}", systemCR3))))
        .Times(1);
    EXPECT_CALL(*mockEventStream,
                sendProcessEvent(::grpc::ProcessState::Started,
                                 StrEq(process248.imageFileName),
                                 process248.processId,
                                 StrEq(fmt::format("{:x}", process248.cr3))))
        .Times(1);

    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());
}

TEST_F(ActiveProcessesSupervisorFixture, addNewProcess_process332_processAdded)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());
    setupProcessWithLink(process332, process0.eprocessBase);

    EXPECT_NO_THROW(activeProcessesSupervisor->addNewProcess(process332.eprocessBase));

    std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> activeProcesses;
    EXPECT_NO_THROW(activeProcesses = activeProcessesSupervisor->getActiveProcesses());
    EXPECT_THAT(*activeProcesses, Contains(IsEqualProcess(process332)));
}

TEST_F(ActiveProcessesSupervisorFixture, addNewProcess_process332_processStartEventGenerated)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());
    setupProcessWithLink(process332, process0.eprocessBase);

    EXPECT_CALL(*mockEventStream,
                sendProcessEvent(::grpc::ProcessState::Started,
                                 StrEq(process332.imageFileName),
                                 process332.processId,
                                 StrEq(fmt::format("{:x}", process332.cr3))))
        .Times(1);
    EXPECT_NO_THROW(activeProcessesSupervisor->addNewProcess(process332.eprocessBase));
}

TEST_F(ActiveProcessesSupervisorFixture, removeActiveProcess_presentProcess_processRemoved)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());

    EXPECT_NO_THROW(activeProcessesSupervisor->removeActiveProcess(process248.eprocessBase));

    std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> activeProcesses;
    EXPECT_NO_THROW(activeProcesses = activeProcessesSupervisor->getActiveProcesses());
    EXPECT_THAT(*activeProcesses, Not(Contains(IsEqualProcess(process248))));
}

TEST_F(ActiveProcessesSupervisorFixture, removeActiveProcess_presentProcess_processTerminationEventGenerated)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());

    EXPECT_CALL(*mockEventStream,
                sendProcessEvent(::grpc::ProcessState::Terminated,
                                 StrEq(process248.imageFileName),
                                 process248.processId,
                                 StrEq(fmt::format("{:x}", process248.cr3))))
        .Times(1);
    EXPECT_NO_THROW(activeProcessesSupervisor->removeActiveProcess(process248.eprocessBase));
}

TEST_F(ActiveProcessesSupervisorFixture, removeNotActiveProcess_inactiveProcess_noChange)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());

    EXPECT_NO_THROW(activeProcessesSupervisor->removeActiveProcess(process332.eprocessBase));

    std::unique_ptr<std::vector<std::shared_ptr<ActiveProcessInformation>>> activeProcesses;
    EXPECT_NO_THROW(activeProcesses = activeProcessesSupervisor->getActiveProcesses());
    EXPECT_THAT(*activeProcesses, UnorderedElementsAre(IsEqualProcess(process4), IsEqualProcess(process248)));
}

TEST_F(ActiveProcessesSupervisorFixture, getProcessInformationByPid_validPid_correctProcessInformation)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());

    std::shared_ptr<ActiveProcessInformation> processInformation;
    EXPECT_NO_THROW(processInformation = activeProcessesSupervisor->getProcessInformationByPid(process248.processId));

    EXPECT_THAT(processInformation, IsEqualProcess(process248));
}

TEST_F(ActiveProcessesSupervisorFixture, getProcessInformationByPid_notPresentPid_invalidArgumentException)
{
    EXPECT_NO_THROW(activeProcessesSupervisor->initialize());

    EXPECT_THROW(auto processInformation = activeProcessesSupervisor->getProcessInformationByPid(unusedPid),
                 std::invalid_argument);
}
