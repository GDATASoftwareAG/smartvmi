#ifndef BUILD_VERSION
#define BUILD_VERSION "TestBuild"
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION "0.0.0"
#endif

#include "Cmdline.h"
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
#include "vmi/InterruptFactory.h"
#include "vmi/LibvmiInterface.h"
#include "vmi/VmiException.h"
#include <boost/di.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <thread>

int main(int argc, const char* argv[])
{
    int exitCode = 0;

    std::shared_ptr<ILogging> logging;
    std::shared_ptr<IEventStream> eventStream;
    std::unique_ptr<ILogger> logger;

    try
    {
        Cmdline cmd{};
        cmd.parse(argc, argv);

        auto enableGRPCServer = cmd.gRPCListenAddressArgument.isSet();
        auto configFilePath = cmd.configFileArgument.getValue();

        const auto injector = boost::di::make_injector(
            boost::di::bind<IConfigParser>().to<ConfigYAMLParser>(),
            boost::di::bind<ILibvmiInterface>().to<LibvmiInterface>(),
            boost::di::bind<ISingleStepSupervisor>().to<SingleStepSupervisor>(),
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
                ::grpc::new_server(cmd.gRPCListenAddressArgument.getValue(), cmd.enableDebugArgument.isSet()))),
            boost::di::bind<::rust::Box<::logging::console::ConsoleLoggerBuilder>>().to(
                std::make_shared<::rust::Box<::logging::console::ConsoleLoggerBuilder>>(
                    ::logging::console::new_console_logger_builder())));

        // We need to init the configuration before anything else so that our loggers will be setup correctly.
        auto configInterface = injector.create<std::shared_ptr<IConfigParser>>();
        configInterface->extractConfiguration(configFilePath);
        if (cmd.domainNameArgument.isSet())
        {
            configInterface->setVmName(cmd.domainNameArgument.getValue());
        }
        if (cmd.kvmiSocketArgument.isSet())
        {
            configInterface->setSocketPath(cmd.kvmiSocketArgument.getValue());
        }
        if (cmd.resultsDirectoryArgument.isSet())
        {
            configInterface->setResultsDirectory(cmd.resultsDirectoryArgument.getValue());
        }
        if (cmd.logLevelArgument.isSet())
        {
            configInterface->setLogLevel(cmd.logLevelArgument.getValue());
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
        vmiHub->run(cmd.pluginArgs);
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
