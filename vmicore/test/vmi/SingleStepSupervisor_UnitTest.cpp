#include "../../src/vmi/SingleStepSupervisor.h"
#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include <gtest/gtest.h>

using testing::_;
using testing::AtLeast;
using testing::NiceMock;
using testing::Return;

TEST(SingleStepSupervisorTest, constructor_multipleInstances_throwsRuntimeError)
{
    std::shared_ptr<NiceMock<MockLogging>> mockLogging = std::make_shared<NiceMock<MockLogging>>();
    ON_CALL(*mockLogging, newNamedLogger(_))
        .WillByDefault([](const std::string& /*name*/) { return std::make_unique<MockGRPCLogger>(); });

    std::shared_ptr<ILibvmiInterface> vmiInterface = std::make_shared<MockLibvmiInterface>();
    SingleStepSupervisor firstInstance(vmiInterface, mockLogging);

    EXPECT_THROW(SingleStepSupervisor secondInstance(vmiInterface, mockLogging), std::runtime_error);
}

class SingleStepSupvervisorValidStateFixture : public testing::Test
{
  protected:
    using singleStepFunction_t = std::function<void(vmi_event_t*)>;

    std::shared_ptr<MockLibvmiInterface> vmiInterface = std::make_shared<MockLibvmiInterface>();
    std::unique_ptr<SingleStepSupervisor> singleStepSupervisor;
    std::shared_ptr<testing::internal::MockFunction<void(vmi_event_t*)>> mockFunction =
        std::make_shared<testing::MockFunction<void(vmi_event_t*)>>();
    std::shared_ptr<NiceMock<MockLogging>> mockLogging = std::make_shared<NiceMock<MockLogging>>();
    singleStepFunction_t callbackFunction =
        SingleStepSupervisor::createSingleStepCallback(mockFunction, &testing::MockFunction<void(vmi_event_t*)>::Call);
    uint numberOfTestVcpus = 1;
    uint testVcpuId = 0;

    void SetUp() override
    {
        ON_CALL(*mockLogging, newNamedLogger(_))
            .WillByDefault([](const std::string& /*name*/) { return std::make_unique<MockGRPCLogger>(); });

        ON_CALL(*vmiInterface, getNumberOfVCPUs()).WillByDefault(Return(numberOfTestVcpus));
        singleStepSupervisor = std::make_unique<SingleStepSupervisor>(vmiInterface, mockLogging);
        singleStepSupervisor->initializeSingleStepEvents();
    }
};

TEST_F(SingleStepSupvervisorValidStateFixture, setSingleStepCallback_callbackTargetExpired_doesNotThrow)
{
    singleStepSupervisor->setSingleStepCallback(testVcpuId, callbackFunction);
    mockFunction.reset();
    vmi_event_t testEvent{};
    testEvent.vcpu_id = testVcpuId;

    EXPECT_NO_THROW(SingleStepSupervisor::_defaultSingleStepCallback(0, &testEvent));
}

TEST_F(SingleStepSupvervisorValidStateFixture, setSingleStepCallback_validCallbackTarget_triggersCallback)
{
    singleStepSupervisor->setSingleStepCallback(testVcpuId, callbackFunction);
    vmi_event_t testEvent{};
    testEvent.vcpu_id = testVcpuId;

    EXPECT_CALL(*mockFunction, Call(_)).Times(1);
    EXPECT_NO_THROW(SingleStepSupervisor::_defaultSingleStepCallback(0, &testEvent));
}

TEST_F(SingleStepSupvervisorValidStateFixture,
       setSingleStepCallback_callbackAlreadyRegisteredForCurrentVcpu_throwsVmiException)
{
    singleStepSupervisor->setSingleStepCallback(testVcpuId, callbackFunction);

    EXPECT_THROW(singleStepSupervisor->setSingleStepCallback(testVcpuId, callbackFunction), VmiException);
}

TEST_F(SingleStepSupvervisorValidStateFixture, setSingleStepCallback_validCallbackTarget_stopSingleStepForVCPU)
{
    singleStepSupervisor->setSingleStepCallback(testVcpuId, callbackFunction);
    vmi_event_t testEvent{};
    testEvent.vcpu_id = testVcpuId;

    EXPECT_CALL(*vmiInterface, stopSingleStepForVcpu(&testEvent, testVcpuId)).Times(AtLeast(1));
    EXPECT_NO_THROW(SingleStepSupervisor::_defaultSingleStepCallback(0, &testEvent));
}
