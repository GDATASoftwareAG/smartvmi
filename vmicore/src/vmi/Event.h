#ifndef VMICORE_EVENT_H
#define VMICORE_EVENT_H

#include "../io/ILogging.h"
#include "../io/grpc/GRPCLogger.h"
#include "LibvmiInterface.h"
#include <cstdint>
#include <libvmi/events.h>
#include <memory>
#include <utility>

namespace VmiCore
{
    class Event
    {
      public:
        explicit Event(std::shared_ptr<ILibvmiInterface> vmiInterface, std::unique_ptr<ILogger> logger)
            : vmiInterface(std::move(vmiInterface)), logger(std::move(logger)){};

        virtual ~Event() = default;

      protected:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<ILogger> logger;

        virtual void initialize() = 0;

        virtual void enableEvent() = 0;

        virtual void disableEvent() = 0;

        virtual void teardown() = 0;
    };
}

#endif // VMICORE_EVENT_H
