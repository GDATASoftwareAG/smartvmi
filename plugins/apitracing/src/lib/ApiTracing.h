#ifndef APITRACING_APITRACING_H
#define APITRACING_APITRACING_H

#include "Tracer.h"
#include <vmicore/plugins/IPlugin.h>

namespace ApiTracing
{
    class ApiTracing : public VmiCore::Plugin::IPlugin
    {
      public:
        ApiTracing(VmiCore::Plugin::PluginInterface* pluginInterface,
                   const VmiCore::Plugin::IPluginConfig& config,
                   std::vector<std::string>& args);

        ~ApiTracing() override = default;

        void unload() override;

      private:
        std::unique_ptr<VmiCore::ILogger> logger;
        std::shared_ptr<Tracer> tracer;
    };
}

#endif // APITRACING_APITRACING_H
