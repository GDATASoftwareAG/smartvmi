#include "../../../src/os/ActiveProcessInformation.h"
#include "../../../src/os/windows/ActiveProcessesSupervisor.h"
#include <gmock/gmock.h>

class MockActiveProcessesSupervisor : public IActiveProcessesSupervisor
{
  public:
    MOCK_METHOD(void, initialize, (), (override));

    MOCK_METHOD(std::shared_ptr<ActiveProcessInformation>, getProcessInformationByPid, (pid_t), (const override));

    MOCK_METHOD(std::shared_ptr<ActiveProcessInformation>, getProcessInformationByBase, (uint64_t), (const override));

    MOCK_METHOD(void, addNewProcess, (uint64_t), (override));

    MOCK_METHOD(void, removeActiveProcess, (uint64_t), (override));

    MOCK_METHOD(std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>,
                getActiveProcesses,
                (),
                (const override));
};
