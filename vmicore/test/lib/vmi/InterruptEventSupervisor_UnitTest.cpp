#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include "../os/windows/mock_ActiveProcessesSupervisor.h"
#include "mock_LibvmiInterface.h"
#include "mock_SingleStepSupervisor.h"
#include <GlobalControl.h>
#include <gtest/gtest.h>
#include <os/PagingDefinitions.h>
#include <plugins/PluginSystem.h>
#include <vmi/VmiException.h>
#include <vmicore/vmi/IBreakpoint.h>

using testing::_;
using testing::AnyNumber;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::SaveArg;

namespace VmiCore
{
    using breakpointCallbackFunction_t = std::function<BpResponse(IInterruptEvent&)>;
    using singleStepCallbackFunction_t = std::function<void(vmi_event_t*)>;

    namespace
    {
        constexpr uint8_t REINJECT_INTERRUPT = 1;
        constexpr uint8_t INT3_BREAKPOINT = 0xCC;
        constexpr vmi_instance* vmiInstanceStub = nullptr;
        constexpr uint64_t testVA = 4321 * PagingDefinitions::pageSizeInBytes,
                           testVA2 = 8765 * PagingDefinitions::pageSizeInBytes;
        constexpr uint64_t testPA = 1234 * PagingDefinitions::pageSizeInBytes,
                           testPA2 = 5678 * PagingDefinitions::pageSizeInBytes;
        constexpr uint64_t testSystemCr3 = 0xaaa;
        constexpr uint64_t testOriginalMemoryContent = 0xFE, testOriginalMemoryContent2 = 0xFF;
        constexpr uint64_t expectedR8 = 0x123;
        constexpr uint32_t testVcpuId = 0;
        constinit x86_registers_t x86Regs{
            .r8 = expectedR8,
            .cr3 = testSystemCr3,
        };
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
        std::shared_ptr<testing::internal::MockFunction<BpResponse(IInterruptEvent&)>> mockFunction =
            std::make_shared<testing::MockFunction<BpResponse(IInterruptEvent&)>>();
        breakpointCallbackFunction_t breakpointCallback = Breakpoint::createBreakpointCallback(
            std::weak_ptr(mockFunction), &testing::MockFunction<BpResponse(IInterruptEvent&)>::Call);
        std::shared_ptr<NiceMock<MockLogging>> mockLogging = std::make_shared<NiceMock<MockLogging>>();
        std::shared_ptr<InterruptEventSupervisor> interruptEventSupervisor;
        vmi_event_t* interruptSupervisorInternalEvent = nullptr;

        std::shared_ptr<IBreakpoint>
        createBreakpointFromMockFunction(const std::shared_ptr<testing::MockFunction<BpResponse(IInterruptEvent&)>>& mf)
        {
            auto occupantCallback = Breakpoint::createBreakpointCallback(
                std::weak_ptr(
                    static_cast<std::shared_ptr<testing::internal::MockFunction<BpResponse(IInterruptEvent&)>>>(mf)),
                &testing::MockFunction<BpResponse(IInterruptEvent&)>::Call);
            return interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, occupantCallback);
        }

        void setupBreakpoint(addr_t eventVA, addr_t eventPA, addr_t eventCr3, uint8_t memoryContent = 0xFE)
        {
            ON_CALL(*vmiInterface, convertVAToPA(eventVA, eventCr3)).WillByDefault(Return(eventPA));
            ON_CALL(*vmiInterface, read8PA(eventPA)).WillByDefault(Return(memoryContent));
        }

        vmi_event_t* setupInterruptEvent(addr_t eventVA, addr_t eventPA, x86_regs& regs, uint32_t vcpuId = 0)
        {
            interruptSupervisorInternalEvent->vcpu_id = vcpuId;
            interruptSupervisorInternalEvent->interrupt_event.gla = eventVA;
            interruptSupervisorInternalEvent->interrupt_event.gfn = eventPA >> PagingDefinitions::numberOfPageIndexBits;
            interruptSupervisorInternalEvent->interrupt_event.offset = eventPA & PagingDefinitions::pageOffsetMask;
            interruptSupervisorInternalEvent->x86_regs = &regs;

            return interruptSupervisorInternalEvent;
        }

        void SetUp() override
        {
            // Required for InterruptGuard
            ON_CALL(*vmiInterface, readXVA(_, _, _)).WillByDefault(Return(true));
            ON_CALL(*mockLogging, newNamedLogger(_))
                .WillByDefault([](std::string_view) { return std::make_unique<MockGRPCLogger>(); });

            // Gain access to internal interrupt event within InterruptEventSupervisor
            ON_CALL(*vmiInterface, registerEvent(_))
                .WillByDefault(
                    [&interruptSupervisorInternalEvent = interruptSupervisorInternalEvent](vmi_event_t& event)
                    {
                        if (event.type == VMI_EVENT_INTERRUPT)
                        {
                            interruptSupervisorInternalEvent = &event;
                        }
                    });
            interruptEventSupervisor =
                std::make_shared<InterruptEventSupervisor>(vmiInterface, singleStepSupervisor, mockLogging);
            interruptEventSupervisor->initialize();

            GlobalControl::init(std::make_unique<NiceMock<MockGRPCLogger>>(),
                                std::make_shared<NiceMock<MockEventStream>>());
        }

        void TearDown() override
        {
            interruptEventSupervisor->teardown();
            GlobalControl::uninit();
        }
    };

    TEST_F(InterruptEventFixture, createBreakpoint_int3AtTargetPA_vmiException)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, INT3_BREAKPOINT);

        EXPECT_THROW(interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallbackFunction_t{}),
                     VmiException);
    }

    TEST_F(InterruptEventFixture, createBreakpoint_validParameters_doesNotThrow)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        EXPECT_CALL(*vmiInterface, write8PA(testPA, INT3_BREAKPOINT)).Times(1).RetiresOnSaturation();

        EXPECT_NO_THROW(interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback));
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_twoEventsRegistered_bothCallbacksCalled)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto occupantMockFunction = std::make_shared<testing::MockFunction<BpResponse(IInterruptEvent&)>>();
        auto _occupant = createBreakpointFromMockFunction(occupantMockFunction);
        auto conflictingInterruptMockFunction = std::make_shared<testing::MockFunction<BpResponse(IInterruptEvent&)>>();
        auto _conflictingInterrupt = createBreakpointFromMockFunction(conflictingInterruptMockFunction);

        EXPECT_CALL(*occupantMockFunction, Call(_)).Times(1);
        EXPECT_CALL(*conflictingInterruptMockFunction, Call(_)).Times(1);
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs);

        InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent);
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_eventReadingR8_CorrectContent)
    {
        uint64_t result = 0;
        auto testCallbackReadsR8 = [&result](IInterruptEvent& event)
        {
            result = event.getR8();
            return BpResponse::Continue;
        };
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto _event = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, testCallbackReadsR8);
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs);

        InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent);

        EXPECT_EQ(expectedR8, result);
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_interruptTriggeredButCallbackExpired_doesNotThrow)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto _interruptEvent = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs);

        mockFunction.reset();

        EXPECT_NO_THROW(InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent));
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_nonRegisteredPA_reinjectsEvent)
    {
        uint64_t unknownVA = 0;
        uint64_t unknownPA = 0;
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto _interruptEvent = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto* interruptEvent = setupInterruptEvent(unknownVA, unknownPA, x86Regs);

        EXPECT_NO_THROW(InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent));

        EXPECT_EQ(interruptEvent->interrupt_event.reinject, REINJECT_INTERRUPT);
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_interruptEventTriggered_disablesEvent)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto _breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1).RetiresOnSaturation();
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs);

        EXPECT_NO_THROW(InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent));
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_singleStepTriggered_reenablesEvent)
    {
        vmi_event_t singleStepEvent{
            .data = reinterpret_cast<void*>(testPA),
            .vcpu_id = testVcpuId,
        };
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto _breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        singleStepCallbackFunction_t singleStepCallback;
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs, testVcpuId);
        EXPECT_CALL(*singleStepSupervisor, setSingleStepCallback(testVcpuId, _, _))
            .WillOnce(SaveArg<1>(&singleStepCallback));
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        EXPECT_CALL(*vmiInterface, write8PA(testPA, INT3_BREAKPOINT)).Times(1).RetiresOnSaturation();

        EXPECT_NO_THROW(InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent));
        ASSERT_TRUE(singleStepCallback);
        EXPECT_NO_THROW(singleStepCallback(&singleStepEvent));
    }

    TEST_F(InterruptEventFixture, _defaultInterruptCallback_interruptEventTriggered_returnsEventResponseNone)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        auto _breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto* interruptEvent = setupInterruptEvent(testVA, testPA, x86Regs);

        EXPECT_EQ(InterruptEventSupervisor::_defaultInterruptCallback(vmiInstanceStub, interruptEvent),
                  VMI_EVENT_RESPONSE_NONE);
    }

    class InterruptEventFixtureWithoutInterruptEventSupervisorTeardown : public InterruptEventFixture
    {
        void TearDown() override
        {
            GlobalControl::uninit();
        }
    };

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown, teardown_activeInterrupt_vmPausedAndResumed)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto _breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        testing::Sequence s1;
        EXPECT_CALL(*vmiInterface, pauseVm()).Times(1).InSequence(s1);
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1).InSequence(s1);
        EXPECT_CALL(*vmiInterface, resumeVm()).Times(1).InSequence(s1);

        interruptEventSupervisor->teardown();
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           teardown_twoActiveInterrupts_interruptsDisabled)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        setupBreakpoint(testVA2, testPA2, testSystemCr3, testOriginalMemoryContent2);
        auto _breakpoint1 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto _breakpoint2 = interruptEventSupervisor->createBreakpoint(testVA2, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1);
        EXPECT_CALL(*vmiInterface, write8PA(testPA2, testOriginalMemoryContent2)).Times(1);

        interruptEventSupervisor->teardown();
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           teardown_twoActiveInterrupts_interruptGuardsDisabled)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        setupBreakpoint(testVA2, testPA2, testSystemCr3);
        auto _breakpoint1 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto _breakpoint2 = interruptEventSupervisor->createBreakpoint(testVA2, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, clearEvent(_, _)).Times(AnyNumber());
        EXPECT_CALL(*vmiInterface,
                    clearEvent(IsCorrectMemEvent(testPA >> PagingDefinitions::numberOfPageIndexBits), false))
            .Times(1);
        EXPECT_CALL(*vmiInterface,
                    clearEvent(IsCorrectMemEvent(testPA2 >> PagingDefinitions::numberOfPageIndexBits), false))
            .Times(1);

        interruptEventSupervisor->teardown();
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_breakpointRemovedTwoTimes_noThrow)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        breakpoint->remove();

        EXPECT_NO_THROW(breakpoint->remove());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_singleBreakpoint_vmPausedAndResumed)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        testing::Sequence s1;
        EXPECT_CALL(*vmiInterface, pauseVm()).Times(1).InSequence(s1);
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1).InSequence(s1);
        EXPECT_CALL(*vmiInterface, resumeVm()).Times(1).InSequence(s1);

        interruptEventSupervisor->deleteBreakpoint(breakpoint.get());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_noPendingEvents_eventsListenNotCalled)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        ON_CALL(*vmiInterface, areEventsPending()).WillByDefault(Return(0));
        EXPECT_CALL(*vmiInterface, eventsListen(_)).Times(0);

        interruptEventSupervisor->deleteBreakpoint(breakpoint.get());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_pendingEventsPresent_eventsListenCalled)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto breakpoint = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        ON_CALL(*vmiInterface, areEventsPending()).WillByDefault(Return(1));
        EXPECT_CALL(*vmiInterface, eventsListen(_)).Times(1);

        interruptEventSupervisor->deleteBreakpoint(breakpoint.get());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_twoBreakpointsOnSameAddress_interruptNotOverwrittenInMemory)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        auto breakpoint1 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto _breakpoint2 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(0);

        interruptEventSupervisor->deleteBreakpoint(breakpoint1.get());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_twoBreakpointsOnSameAddress_VmNotPaused)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3, testOriginalMemoryContent);
        auto breakpoint1 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto _breakpoint2 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, pauseVm()).Times(0);
        EXPECT_CALL(*vmiInterface, resumeVm()).Times(0);

        interruptEventSupervisor->deleteBreakpoint(breakpoint1.get());
    }

    TEST_F(InterruptEventFixtureWithoutInterruptEventSupervisorTeardown,
           deleteBreakpoint_twoBreakpointsOnSameAddressRemoved_interruptRemovedFromMemory)
    {
        setupBreakpoint(testVA, testPA, testSystemCr3);
        EXPECT_CALL(*vmiInterface, write8PA(_, _)).Times(AnyNumber());
        auto breakpoint1 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        auto breakpoint2 = interruptEventSupervisor->createBreakpoint(testVA, testSystemCr3, breakpointCallback);
        EXPECT_CALL(*vmiInterface, write8PA(testPA, testOriginalMemoryContent)).Times(1);

        interruptEventSupervisor->deleteBreakpoint(breakpoint1.get());
        interruptEventSupervisor->deleteBreakpoint(breakpoint2.get());
    }
}
