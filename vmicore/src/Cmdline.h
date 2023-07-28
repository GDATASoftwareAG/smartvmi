#ifndef VMICORE_CMDLINE_H
#define VMICORE_CMDLINE_H

#include <filesystem>
#include <functional> // std::equal_to
#include <iostream>
#include <map>
#include <string>
#include <tclap/ArgException.h>
#include <vector>

std::istream& operator>>(std::istream& is, std::pair<std::string, std::string>& p);

std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<std::string, std::string>>& p);

// This header uses the custom stream operators defined above and therefore has to be included after their declaration.
#include <tclap/CmdLine.h>

class Cmdline
{
  public:
    TCLAP::CmdLine cmd{"VMI tool for automated malware behaviour extraction.", ' ', PROGRAM_VERSION};
    TCLAP::ValueArg<std::string> configFileArgument{"c",
                                                    "config",
                                                    "YAML file containing the config for the VMI analysis.",
                                                    false,
                                                    std::filesystem::path(SYSCONF_DIR) / "vmicore.yml",
                                                    "config_file.yml",
                                                    cmd};
    TCLAP::ValueArg<std::string> domainNameArgument{
        "n", "name", "Name of the domain to introspect.", false, "", "domain_name", cmd};
    TCLAP::ValueArg<std::filesystem::path> kvmiSocketArgument{
        "s", "socket", "KVMi socket path {required for introspecting on kvm}.", false, "", "/path/to/socket", cmd};
    TCLAP::ValueArg<std::string> resultsDirectoryArgument{
        "r", "results", "Path to top level directory for results.", false, "./results", "results_directory", cmd};
    TCLAP::ValueArg<std::string> gRPCListenAddressArgument{
        "g", "grpc-listen-addr", "Listen address for grpc server.", false, "", "listen_address", cmd};
    TCLAP::ValueArg<std::string> logLevelArgument{"l",
                                                  "log-level",
                                                  "Log level to use - [debug, info (default), warning, error].",
                                                  false,
                                                  "info",
                                                  "Log level",
                                                  cmd};
    TCLAP::SwitchArg enableDebugArgument{"", "grpc-debug", "Enable additional console logs for gRPC mode.", cmd};
    TCLAP::MultiArg<std::pair<std::string, std::string>> pluginArguments{
        "p", "plugin", "Plugin command line parameters.", false, "plugin: cmdline args", cmd};
    std::map<std::string, std::vector<std::string>, std::less<>> pluginArgs{};

    void parse(int argc, const char** argv);

  private:
    static std::map<std::string, std::vector<std::string>, std::less<>>
    convertPluginArgValuesToArgVectors(const std::vector<std::pair<std::string, std::string>>& pluginArgs);

    static std::vector<std::string> splitArgs(const std::string& arg);
};

#endif // VMICORE_CMDLINE_H
