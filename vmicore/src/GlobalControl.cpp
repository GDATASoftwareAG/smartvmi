#include "GlobalControl.h"

namespace VmiCore
{
    namespace
    {
        std::unique_ptr<ILogger> staticLogger;
        std::shared_ptr<IEventStream> staticEventStream;
    }

    namespace GlobalControl
    {
        volatile bool endVmi = false;
        volatile bool postRunPluginAction = false;

        const std::unique_ptr<ILogger>& logger()
        {
            if (staticLogger)
            {
                return staticLogger;
            }
            throw std::runtime_error("No Logger present");
        }

        const std::shared_ptr<IEventStream>& eventStream()
        {
            if (staticEventStream)
            {
                return staticEventStream;
            }
            throw std::runtime_error("No Eventstream present");
        }

        void init(std::unique_ptr<ILogger> logger, std::shared_ptr<IEventStream> eventStream)
        {
            staticLogger = std::move(logger);
            staticEventStream = std::move(eventStream);
        }

        void uninit()
        {
            staticLogger.reset();
            staticEventStream.reset();
        }
    }
}
