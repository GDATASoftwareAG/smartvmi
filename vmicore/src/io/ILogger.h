#ifndef VMICORE_ILOGGER_H
#define VMICORE_ILOGGER_H

#include "cxxbridge/rust/cxx.h"
#include "cxxbridge/rust_grpc_server/src/bridge.rs.h"
#include <memory>

#define NEW_LOGGER(log_server) (log_server->newNamedLogger(std::filesystem::path(__FILE__).filename().stem()))
// Has to be the same as in the HiveOperations project to indicate in which file the log should be written.
// pkg->script->vmi->vmi_connector.go
#define WRITE_TO_FILE_TAG "writeToFileTag"

namespace logfield
{
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const std::string& val)
    {
        return ::logging::field_str(key, val);
    }
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const char* val)
    {
        return ::logging::field_str(key, val);
    }
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const bool& val)
    {
        return ::logging::field_bool(key, val);
    }
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const int64_t& val)
    {
        return ::logging::field_i64(key, val);
    }
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const uint64_t& val)
    {
        return ::logging::field_uint64(key, val);
    }
    inline ::rust::Box<::logging::LogField> create(const std::string& key, const double& val)
    {
        return ::logging::field_float64(key, val);
    }
}

class ILogger
{
  public:
    virtual ~ILogger() = default;
    virtual void bind(std::initializer_list<::rust::Box<::logging::LogField>> fields) = 0;

    virtual void debug(std::string message) const = 0;
    virtual void debug(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const = 0;

    virtual void info(std::string message) const = 0;
    virtual void info(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const = 0;

    virtual void warning(std::string message) const = 0;
    virtual void warning(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const = 0;

    virtual void error(std::string message) const = 0;
    virtual void error(std::string message, std::initializer_list<::rust::Box<::logging::LogField>> fields) const = 0;

  protected:
    ILogger() = default;
};

#endif // VMICORE_ILOGGER_H
