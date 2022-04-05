#include "../../src/GlobalControl.h"
#include "../../src/os/PagingDefinitions.h"
#include "../../src/vmi/InterruptEvent.h"
#include "../../src/vmi/InterruptFactory.h"
#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include "mock_LibvmiInterface.h"
#include "mock_SingleStepSupervisor.h"
#include <gtest/gtest.h>

using testing::_;
using testing::AnyNumber;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::SaveArg;

using interruptCallbackFunction_t = std::function<InterruptEvent::InterruptResponse(InterruptEvent&)>;
using singleStepCallbackFunction_t = std::function<void(vmi_event_t*)>;

namespace
{
    vmi_instance constexpr* vmiInstance_stub = nullptr;
    constexpr uint64_t testVA = 4321 * PagingDefinitions::pageSizeInBytes,
                       testVA2 = 8765 * PagingDefinitions::pageSizeInBytes;
    constexpr uint64_t testPA = 1234 * PagingDefinitions::pageSizeInBytes,
                       testPA2 = 5678 * PagingDefinitions::pageSizeInBytes;
    constexpr uint64_t testSystemCr3 = 0xaaa;
    constexpr uint64_t testOriginalMemoryContent = 0xFE, testOriginalMemoryContent2 = 0xFF;
}

MATCHER_P(IsCorrectMemEvent, expectedGfn, "")
{
    if (arg.type != VMI_EVENT_MEMORY)
    {
        *result_listener << "\nNot a memory event: type = " << arg.type;
        return false;
    }
    if (expectedGfn != arg.mem_event.gfn)
    {
        *result_listener << "\nTarget gfn not equal:";
        *result_listener << "\nExpected: " << expectedGfn;
        *result_listener << "\nActual: " << arg.mem_event.gfn;
        return false;
    }
    return true;
}

class InterruptEventFixture : public testing::Test
{
  protected:
    std::shared_ptr<MockLibvmiInterface> vmiInterface = std::make_shared<MockLibvmiInterface>();
    std::shared_ptr<MockSingleStepSupervisor> singleStepSupervisor = std::make_shared<MockSingleStepSupervisor>();
    std::shared_ptr<testing::internal::MockFunction<InterruptEvent::InterruptResponse(InterruptEvent&)>> mockFunction =
        std::make_shared<testing::MockFunction<InterruptEvent::InterruptResponse(InterruptEvent&)>>();
    interruptCallbackFunction_t interruptCallback = InterruptEvent::createInterruptCallback(
        std::weak_ptr(mockFunction), &testing::MockFunction<InterruptEvent::InterruptResponse(InterruptEvent&)>::Call);
    std::shared_ptr<NiceMock<MockLogging>> mockLogging = std::make_shared<NiceMock<MockLogging>>();
    InterruptFactory interruptFactory = InterruptFactory(vmiInterface, singleStepSupervisor, mockLogging);

    void SetUp() override
    {
        ON_CALL(*vmiInterface, convertVAToPA(testVA, testSystemCr3)).WillByDefault(Return(testPA));
        ON_CALL(*vmiInterface, convertVAToPA(testVA2, testSystemCr3)).WillByDefault(Return(testPA2));
        ON_CALL(*vmiInterface, read8PA(testPA)).WillByDefault(Return(testOriginalMemoryContent));
        ON_CALL(*vmiInterface, read8PA(testPA2)).WillByDefault(Return(testOriginalMemoryContent2));
        ON_CALL(*vmiInterface, readXVA(_, _, _)).WillByDefault(Return(true));
        ON_CALL(*mockLogging, newNamedLogger(_))
            .WillByDefault([](const std::string& /*name*/) { return std::make_unique<MockGRPCLogger>(); });
        interruptFactory.initialize();

        GlobalControl::init(std::make_unique<NiceMock<MockGRPCLogger>>(),
                            std::make_shared<NiceMock<MockEventStream>>());
    }

    void TearDown() override
    {
        interruptFactory.teardown();
    }
};

TEST_F(InterruptEventFixture, createInterruptEvent_int3AtTargetPA_throwsVmiException)
{
    auto testVAInt3 = 6789;
    auto testPAInt3 = 9876;
    ON_CALL(*vmiInterface, convertVAToPA(testVAInt3, testSystemCr3)).WillByDefault(Return(testPAInt3));
    ON_CALL(*vmiInterface, read8PA(testPAInt3)).WillByDefault(Return(INT3_BREAKPOINT));
    interruptFactory.initialize();

    EXPECT_THROW(interruptFactory.createInterruptEvent(
                     "InterruptName", testVAInt3, testSystemCr3, interruptCallbackFunction_t{}),
                 VmiException);
}

TEST_F(InterruptEventFixture, createInterruptEvent_validParameters_doesNotThrow)
{
    EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
    EXPECT_CALL(*vmiInterface, write8PA(testPA, INT3_BREAKPOINT)).Times(1).RetiresOnSaturation();

    EXPECT_NO_THROW(interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback));
}

TEST_F(InterruptEventFixture, createInterruptEvent_targetPAoccupiedByAnotherInterruptEvent_throwsVmiException)
{
    [[maybe_unused]] auto _occupant =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    EXPECT_CALL(*vmiInterface, read8PA(testPA)).WillOnce(Return(testOriginalMemoryContent));

    EXPECT_THROW(interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback),
                 VmiException);
}

TEST_F(InterruptEventFixture, createInterruptEvent_interruptTriggeredButCallbackExpired_doesNotThrow)
{
    [[maybe_unused]] auto _interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    vmi_event_t event{};
    event.interrupt_event.gfn = 0;
    event.interrupt_event.offset = testPA;
    mockFunction.reset();

    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));
}

TEST_F(InterruptEventFixture, createInterruptEvent_nonRegisteredPA_reinjectsEvent)
{
    [[maybe_unused]] auto _interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    uint64_t unknownPA = 0;
    vmi_event_t event{};
    event.interrupt_event.gfn = 0;
    event.interrupt_event.offset = unknownPA;

    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));

    EXPECT_EQ(event.interrupt_event.reinject, REINJECT_INTERRUPT);
}

class InterruptEventCallbackFixture : public InterruptEventFixture
{
  protected:
    vmi_event_t event{};
    uint32_t testVcpuId = 0;

    void SetUp() override
    {
        InterruptEventFixture::SetUp();
        event.interrupt_event.gfn = 0;
        event.interrupt_event.offset = testPA;
        event.vcpu_id = testVcpuId;
    }
};

TEST_F(InterruptEventCallbackFixture, createInterruptEvent_interruptEventTriggered_triggersCallback)
{
    auto interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);

    EXPECT_CALL(*mockFunction, Call(Ref(*interruptEvent))).Times(1);
    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));
}

TEST_F(InterruptEventCallbackFixture, createInterruptEvent_interruptEventTriggered_disablesEvent)
{
    auto interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
    EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1).RetiresOnSaturation();
    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));
}

TEST_F(InterruptEventCallbackFixture, createInterruptEvent_interruptEventTriggered_returnsEventResponseNone)
{
    auto interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);

    EXPECT_EQ(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event), VMI_EVENT_RESPONSE_NONE);
}

TEST_F(InterruptEventCallbackFixture, createInterruptEvent_singleStepTriggered_togglesSingleStep)
{
    auto interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    singleStepCallbackFunction_t singleStepCallback;
    EXPECT_CALL(*singleStepSupervisor, setSingleStepCallback(testVcpuId, _)).WillOnce(SaveArg<1>(&singleStepCallback));

    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));
    ASSERT_TRUE(singleStepCallback);

    EXPECT_NO_THROW(singleStepCallback(&event));
}

TEST_F(InterruptEventCallbackFixture, createInterruptEvent_singleStepTriggered_reenablesEvent)
{
    auto interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    singleStepCallbackFunction_t singleStepCallback;
    EXPECT_CALL(*singleStepSupervisor, setSingleStepCallback(testVcpuId, _)).WillOnce(SaveArg<1>(&singleStepCallback));

    EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
    EXPECT_CALL(*vmiInterface, write8PA(testPA, INT3_BREAKPOINT)).Times(1).RetiresOnSaturation();
    EXPECT_NO_THROW(InterruptEvent::_defaultInterruptCallback(vmiInstance_stub, &event));
    ASSERT_TRUE(singleStepCallback);

    EXPECT_NO_THROW(singleStepCallback(&event));
}

class InterruptEventFixtureWithoutTeardown : public InterruptEventFixture
{
    void TearDown() override{};
};

TEST_F(InterruptEventFixtureWithoutTeardown, clearInterruptEventHandling_activeInterrupt_vmPausedAndResumed)
{
    EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
    testing::Sequence s1;
    EXPECT_CALL(*vmiInterface, pauseVm()).Times(1).InSequence(s1);
    EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1).InSequence(s1);
    EXPECT_CALL(*vmiInterface, resumeVm()).Times(1).InSequence(s1);

    [[maybe_unused]] auto _interruptEvent =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);

    InterruptEvent::clearInterruptEventHandling(*vmiInterface);
}

TEST_F(InterruptEventFixtureWithoutTeardown, clearInterruptEventHandling_twoActiveInterrupts_interruptsDisabled)
{
    EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
    EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1);
    EXPECT_CALL(*vmiInterface, write8PA(testPA2, testOriginalMemoryContent2)).Times(1);

    [[maybe_unused]] auto _interruptEvent1 =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    [[maybe_unused]] auto _interruptEvent2 =
        interruptFactory.createInterruptEvent("InterruptName2", testVA2, testSystemCr3, interruptCallback);

    InterruptEvent::clearInterruptEventHandling(*vmiInterface);
}

TEST_F(InterruptEventFixtureWithoutTeardown, clearInterruptEventHandling_twoActiveInterrupts_interruptGuardsDisabled)
{
    EXPECT_CALL(*vmiInterface, clearEvent(_, _)).Times(AnyNumber());
    EXPECT_CALL(*vmiInterface, clearEvent(IsCorrectMemEvent(testPA >> PagingDefinitions::numberOfPageIndexBits), true))
        .Times(1);
    EXPECT_CALL(*vmiInterface, clearEvent(IsCorrectMemEvent(testPA2 >> PagingDefinitions::numberOfPageIndexBits), true))
        .Times(1);

    [[maybe_unused]] auto _interruptEvent1 =
        interruptFactory.createInterruptEvent("InterruptName", testVA, testSystemCr3, interruptCallback);
    [[maybe_unused]] auto _interruptEvent2 =
        interruptFactory.createInterruptEvent("InterruptName2", testVA2, testSystemCr3, interruptCallback);

    InterruptEvent::clearInterruptEventHandling(*vmiInterface);
}
