#include "GRPCServer.h"
#include "../RustHelper.h"
#include "GRPCLogger.h"
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>

namespace VmiCore
{
    GRPCServer::GRPCServer(std::shared_ptr<rust::Box<grpc::GRPCServer>> server) : server(std::move(server)) {}

    void GRPCServer::start()
    {
        grpcThread.emplace(std::thread([&server = server]() { (*server)->start_server(); }));
    }

    void GRPCServer::stop(const uint64_t& timeoutMillis)
    {
        try
        {
            (*server)->stop_server(timeoutMillis);
            grpcThread.value().join();
        }
        catch (const std::exception& e)
        {
            std::cerr << "failed to shutdown log server: " << e.what() << std::endl;
        }
    }

    std::unique_ptr<ILogger> GRPCServer::newLogger()
    {
        return std::make_unique<GRPCLogger>((*server)->new_logger());
    }

    std::unique_ptr<ILogger> GRPCServer::newNamedLogger(std::string_view name)
    {
        return std::make_unique<GRPCLogger>((*server)->new_named_logger(toRustStr(name)));
    }

    void GRPCServer::setLogLevel(::logging::Level level)
    {
        (*server)->set_log_level(level);
    }

    void GRPCServer::saveBinaryToFile(std::string_view logFileName, const std::vector<uint8_t>& data)
    {
        (*server)->write_message_to_file(toRustStr(logFileName), data);
    }

    void GRPCServer::sendProcessEvent(::grpc::ProcessState processState,
                                      std::string_view processName,
                                      uint32_t processID,
                                      std::string_view cr3)
    {
        (*server)->send_process_event(processState, toRustStr(processName), processID, toRustStr(cr3));
    }

    void GRPCServer::sendBSODEvent(int64_t code)
    {
        (*server)->send_bsod_event(code);
    }

    void GRPCServer::sendReadyEvent()
    {
        (*server)->send_ready_event();
    }

    void GRPCServer::sendTerminationEvent()
    {
        (*server)->send_termination_event();
    }

    void GRPCServer::sendErrorEvent(std::string_view message)
    {
        (*server)->send_error_event(toRustStr(message));
    }

    void GRPCServer::sendInMemDetectionEvent(std::string_view message)
    {
        (*server)->send_in_mem_detection_event(toRustStr(message));
    }
}
