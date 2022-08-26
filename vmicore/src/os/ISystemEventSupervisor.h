#ifndef VMICORE_ISYSTEMEVENTSUPERVISOR_H
#define VMICORE_ISYSTEMEVENTSUPERVISOR_H

namespace VmiCore
{
    class ISystemEventSupervisor
    {
      public:
        virtual ~ISystemEventSupervisor() = default;

        virtual void initialize() = 0;

        virtual void teardown() = 0;

      protected:
        ISystemEventSupervisor() = default;
    };
}

#endif // VMICORE_ISYSTEMEVENTSUPERVISOR_H
