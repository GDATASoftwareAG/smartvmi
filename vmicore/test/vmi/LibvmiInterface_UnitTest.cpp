#include "../../src/vmi/LibvmiInterface.h"
#include "../../src/vmi/VmiException.h"
#include "../io/grpc/mock_GRPCLogger.h"
#include "../io/mock_EventStream.h"
#include "../io/mock_Logging.h"
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::Unused;

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
