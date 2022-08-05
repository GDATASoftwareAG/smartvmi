#ifndef VMICORE_SYSTEMEVENT_H
#define VMICORE_SYSTEMEVENT_H

class ISystemEventSupervisor
{
  public:
    virtual void initialize() = 0;

    virtual void teardown() = 0;
};

#endif // VMICORE_SYSTEMEVENT_H
