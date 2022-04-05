#include "InterruptFactory.h"
#include "../io/grpc/GRPCLogger.h"
#include "InterruptGuard.h"
#include <memory>

InterruptFactory::InterruptFactory(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                   std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                                   std::shared_ptr<ILogging> loggingLib)
    : vmiInterface(std::move(vmiInterface)),
      singleStepSupervisor(std::move(singleStepSupervisor)),
      loggingLib(std::move(loggingLib))
{
}

void InterruptFactory::initialize()
{
    InterruptEvent::initializeInterruptEventHandling(*vmiInterface);
    singleStepSupervisor->initializeSingleStepEvents();
}

void InterruptFactory::teardown()
{
    InterruptEvent::clearInterruptEventHandling(*vmiInterface);
}

std::shared_ptr<InterruptEvent> InterruptFactory::createInterruptEvent(
    const std::string& interruptName,
    uint64_t targetVA,
    uint64_t systemCr3,
    std::function<InterruptEvent::InterruptResponse(InterruptEvent&)> callbackFunction)
{

    auto targetPA = vmiInterface->convertVAToPA(targetVA, systemCr3);

    auto interruptGuard = std::make_unique<InterruptGuard>(
        vmiInterface, loggingLib->newNamedLogger(interruptName), targetVA, targetPA, systemCr3);

    interruptGuard->initialize();

    auto interruptEvent = std::make_shared<InterruptEvent>(vmiInterface,
                                                           targetPA,
                                                           singleStepSupervisor,
                                                           std::move(interruptGuard),
                                                           callbackFunction,
                                                           loggingLib->newNamedLogger(interruptName));

    interruptEvent->initialize();
    return interruptEvent;
}
