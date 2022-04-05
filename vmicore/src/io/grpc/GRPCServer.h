#ifndef VMICORE_GRPCSERVER_H
#define VMICORE_GRPCSERVER_H

#include "../IEventStream.h"
#include "../IFileTransport.h"
#include "../ILogging.h"
#include <memory>
#include <optional>
#include <thread>

class GRPCServer : public ILogging, public IFileTransport, public IEventStream
{
  public:
    explicit GRPCServer(std::shared_ptr<::rust::Box<grpc::GRPCServer>> server);
    ~GRPCServer() override = default;

    void start() override;

    void stop(const uint64_t& timeoutMillis) override;

    std::unique_ptr<ILogger> newLogger() override;

    std::unique_ptr<ILogger> newNamedLogger(const std::string& name) override;

    void setLogLevel(::logging::Level level) override;

    void saveBinaryToFile(const std::string& logFileName, const std::vector<uint8_t>& data) override;

    void sendProcessEvent(::grpc::ProcessState processState,
                          const std::string& processName,
                          uint32_t processID,
                          const std::string& cr3) override;

    void sendBSODEvent(int64_t code) override;

    void sendTerminationEvent() override;

    void sendReadyEvent() override;

    void sendErrorEvent(const std::string& message) override;

    void sendInMemDetectionEvent(const std::string& message) override;

  private:
    std::shared_ptr<::rust::Box<grpc::GRPCServer>> server;
    std::optional<std::thread> grpcThread;
};

#endif // VMICORE_GRPCSERVER_H
