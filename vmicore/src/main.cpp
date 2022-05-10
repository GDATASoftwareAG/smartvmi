#include "GlobalControl.h"
#include "VmiHub.h"
#include "config/ConfigYAMLParser.h"
#include "cxxbridge/rust/cxx.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include "io/IEventStream.h"
#include "io/IFileTransport.h"
#include "io/ILogging.h"
#include "io/console/ConsoleLoggerBuilder.h"
#include "io/console/DummyEventStream.h"
#include "io/grpc/GRPCServer.h"
#include "plugins/PluginSystem.h"
#include "vmi/InterruptFactory.h"
#include "vmi/LibvmiInterface.h"
#include "vmi/VmiException.h"
#include <boost/di.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <tclap/CmdLine.h>
#include <thread>

#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "0.0.0"
#endif

int main(int argc, const char* argv[])
{
    int exitCode = 0;

    TCLAP::CmdLine cmd("VMI tool for automated malware behaviour extraction.", ' ', PROGRAM_VERSION);
    TCLAP::ValueArg<std::string> configFileArgument("c",
                                                    "config",
                                                    "YAML file containing the config for the VMI analysis.",
                                                    false,
                                                    INSTALL_PREFIX "/etc/vmicore.conf",
                                                    "config_file.yml",
                                                    cmd);
    TCLAP::ValueArg<std::string> domainNameArgument(
        "n", "name", "Name of the domain to introspect.", false, "", "domain_name", cmd);
    TCLAP::ValueArg<std::filesystem::path> kvmiSocketArgument(
        "s", "socket", "KVMi socket path (required for introspecting on kvm).", false, "", "/path/to/socket", cmd);
    TCLAP::ValueArg<std::string> resultsDirectoryArgument(
        "r", "results", "Path to top level directory for results.", false, "./results", "results_directory", cmd);
    TCLAP::ValueArg<std::string> gRPCListenAddressArgument(
        "g", "grpc-listen-addr", "Listen address for grpc server.", false, "", "listen_address", cmd);
    TCLAP::ValueArg<std::string> logLevelArgument("l",
                                                  "log-level",
                                                  "Log level to use - [debug, info (default), warning, error].",
                                                  false,
                                                  "info",
                                                  "Log level",
                                                  cmd);
    TCLAP::SwitchArg dumpMemoryArgument("d", "dump", "Dump memory if the inmemory scanner plugin is loaded.", cmd);
    TCLAP::SwitchArg enableDebugArgument("", "grpc-debug", "Enable additional console logs for gRPC mode.", cmd);

    std::shared_ptr<ILogging> logging;
    std::shared_ptr<IEventStream> eventStream;
    std::unique_ptr<ILogger> logger;

    try
    {
        cmd.parse(argc, argv);
        auto enableGRPCServer = gRPCListenAddressArgument.isSet();
        auto configFilePath = configFileArgument.getValue();

        const auto injector = boost::di::make_injector(
            boost::di::bind<IConfigParser>().to<ConfigYAMLParser>(),
            boost::di::bind<ILibvmiInterface>().to<LibvmiInterface>(),
            boost::di::bind<ISingleStepSupervisor>().to<SingleStepSupervisor>(),
            boost::di::bind<IPluginSystem>().to<PluginSystem>(),
            boost::di::bind<IActiveProcessesSupervisor>().to<ActiveProcessesSupervisor>(),
            boost::di::bind<IKernelObjectExtractorWin10>().to<KernelObjectExtractorWin10>(),
            boost::di::bind<IInterruptFactory>().to<InterruptFactory>(),
            boost::di::bind<ILogging>().to(
                [&enableGRPCServer](const auto& injector) -> std::shared_ptr<ILogging>
                {
                    if (enableGRPCServer)
                    {
                        return injector.template create<std::shared_ptr<GRPCServer>>();
                    }
                    return injector.template create<std::shared_ptr<ConsoleLoggerBuilder>>();
                }),
            boost::di::bind<IEventStream>().to(
                [&enableGRPCServer](const auto& injector) -> std::shared_ptr<IEventStream>
                {
                    if (enableGRPCServer)
                    {
                        return injector.template create<std::shared_ptr<GRPCServer>>();
                    }
                    return injector.template create<std::shared_ptr<DummyEventStream>>();
                }),
            boost::di::bind<IFileTransport>().to(
                [&enableGRPCServer](const auto& injector) -> std::shared_ptr<IFileTransport>
                {
                    if (enableGRPCServer)
                    {
                        return injector.template create<std::shared_ptr<GRPCServer>>();
                    }
                    return injector.template create<std::shared_ptr<LegacyLogging>>();
                }),
            boost::di::bind<::rust::Box<::grpc::GRPCServer>>().to(std::make_shared<::rust::Box<::grpc::GRPCServer>>(
                ::grpc::new_server(gRPCListenAddressArgument.getValue(), enableDebugArgument.isSet()))),
            boost::di::bind<::rust::Box<::logging::console::ConsoleLoggerBuilder>>().to(
                std::make_shared<::rust::Box<::logging::console::ConsoleLoggerBuilder>>(
                    ::logging::console::new_console_logger_builder())));

        // We need to init the configuration before anything else so that our loggers will be setup correctly.
        auto configInterface = injector.create<std::shared_ptr<IConfigParser>>();
        configInterface->extractConfiguration(configFilePath);
        if (domainNameArgument.isSet())
        {
            configInterface->setVmName(domainNameArgument.getValue());
        }
        if (kvmiSocketArgument.isSet())
        {
            configInterface->setSocketPath(kvmiSocketArgument.getValue());
        }
        if (resultsDirectoryArgument.isSet())
        {
            configInterface->setResultsDirectory(resultsDirectoryArgument.getValue());
        }
        if (dumpMemoryArgument.isSet())
        {
            auto pluginMap = configInterface->getPlugins();
            auto inMemoryPluginConfig = pluginMap.find("libinmemory.so");
            if (inMemoryPluginConfig != pluginMap.end())
            {
                inMemoryPluginConfig->second->overrideString("dump_memory", "true");
            }
        }
        if (logLevelArgument.isSet())
        {
            configInterface->setLogLevel(logLevelArgument.getValue());
        }

        auto logLevel = ::logging::convert_to_log_level(configInterface->getLogLevel());
        logging = injector.create<std::shared_ptr<ILogging>>();
        logging->setLogLevel(logLevel);
        logging->start();

        logger = logging->newNamedLogger("main");
        logger->info("logging initialised");

        eventStream = injector.create<std::shared_ptr<IEventStream>>();
        auto vmiHub = injector.create<std::shared_ptr<VmiHub>>();

        GlobalControl::init(logging->newLogger(), eventStream);
        vmiHub->run();
    }
    catch (const ::rust::Error&)
    {
        return 2;
    }
    catch (const TCLAP::ArgException& e)
    {
        if (logger)
        {
            logger->error("CLI parse error",
                          {logfield::create("ArgID", e.argId()), logfield::create("Exception", e.error())});
        }
        else
        {
            std::cerr << e.error() << " for arg " << e.argId() << std::endl;
        }
        if (eventStream)
        {
            eventStream->sendErrorEvent(e.error());
        }
        exitCode = 1;
    }
    catch (const std::exception& e)
    {
        if (logger)
        {
            logger->error("Unhandled exception", {logfield::create("Exception", e.what())});
        }
        else
        {
            std::cerr << e.what() << std::endl;
        }
        if (eventStream)
        {
            eventStream->sendErrorEvent(e.what());
        }
        exitCode = 1;
    }

    if (logger)
    {
        logger->info("Done with VMI", {logfield::create("ExitCode", static_cast<int64_t>(exitCode))});
    }
    if (eventStream)
    {
        eventStream->sendTerminationEvent();
    }

    GlobalControl::uninit();
    if (logging)
    {
        logging->stop(5000);
    }

    return exitCode;
}
