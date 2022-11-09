#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include <gtest/gtest.h>
#include <memory>
#include <vmi/LibvmiInterface.h>
#include <vmi/VmiException.h>

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::Unused;

namespace VmiCore
{
    TEST(LibvmiInterfaceTest, constructor_validVmState_doesNotThrow)
    {
        EXPECT_NO_THROW(LibvmiInterface vmiInterface(std::shared_ptr<IConfigParser>(),
                                                     std::make_shared<NiceMock<MockLogging>>(),
                                                     std::make_shared<NiceMock<MockEventStream>>()));
    }

    TEST(LibvmiInterfaceTest, constructor_initializeSecondInstance_throwsRuntimeError)
    {
        LibvmiInterface firstVmiInterface(std::shared_ptr<IConfigParser>(),
                                          std::make_shared<NiceMock<MockLogging>>(),
                                          std::make_shared<NiceMock<MockEventStream>>());

        EXPECT_THROW(LibvmiInterface secondVmiInterface(std::shared_ptr<IConfigParser>(),
                                                        std::make_shared<NiceMock<MockLogging>>(),
                                                        std::make_shared<NiceMock<MockEventStream>>()),
                     std::runtime_error);
    }
}
