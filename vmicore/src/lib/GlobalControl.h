#ifndef VMICORE_GLOBALCONTROL_H
#define VMICORE_GLOBALCONTROL_H

#include "io/IEventStream.h"
#include <memory>
#include <vmicore/io/ILogger.h>

namespace VmiCore::GlobalControl
{
    extern volatile bool endVmi;
    extern volatile bool postRunPluginAction;

    const std::unique_ptr<ILogger>& logger();
    const std::shared_ptr<IEventStream>& eventStream();

    void init(std::unique_ptr<ILogger> logger, std::shared_ptr<IEventStream> eventStream);
    void uninit();
}

#endif // VMICORE_GLOBALCONTROL_H
