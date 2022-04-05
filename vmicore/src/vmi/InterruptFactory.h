#ifndef VMICORE_INTERRUPTFACTORY_H
#define VMICORE_INTERRUPTFACTORY_H

#include "../io/ILogging.h"
#include "InterruptEvent.h"
#include "LibvmiInterface.h"
#include "SingleStepSupervisor.h"

class IInterruptFactory
{
  public:
    virtual ~IInterruptFactory() = default;

    virtual void initialize() = 0;
    virtual void teardown() = 0;
    virtual std::shared_ptr<InterruptEvent>
    createInterruptEvent(const std::string& interruptName,
                         uint64_t targetVA,
                         uint64_t systemCr3,
                         std::function<InterruptEvent::InterruptResponse(InterruptEvent&)> callbackFunction) = 0;

  protected:
    IInterruptFactory() = default;
};

class InterruptFactory : public IInterruptFactory
{
  public:
    explicit InterruptFactory(std::shared_ptr<ILibvmiInterface> vmiInterface,
                              std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor,
                              std::shared_ptr<ILogging> loggingLib);

    ~InterruptFactory() override = default;

    void initialize() override;
    void teardown() override;
    std::shared_ptr<InterruptEvent>
    createInterruptEvent(const std::string& interruptName,
                         uint64_t targetVA,
                         uint64_t systemCr3,
                         std::function<InterruptEvent::InterruptResponse(InterruptEvent&)> callbackFunction) override;

  private:
    std::shared_ptr<ILibvmiInterface> vmiInterface;
    std::shared_ptr<ISingleStepSupervisor> singleStepSupervisor;
    std::shared_ptr<ILogging> loggingLib;
};

#endif // VMICORE_INTERRUPTFACTORY_H
