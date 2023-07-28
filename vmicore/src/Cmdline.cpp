#include "Cmdline.h"
#include <fmt/core.h>

std::istream& operator>>(std::istream& is, std::pair<std::string, std::string>& p)
{

    std::string paramValue;
    std::getline(is, paramValue, {});
    auto index = paramValue.find(':');

    if (index == std::string::npos)
    {
        throw TCLAP::ArgParseException("Malformed parameter: " + paramValue);
    }

    p.first = paramValue.substr(0, index);
    if (index < paramValue.length() - 1)
    {
        p.second = paramValue.substr(index + 1);
    }

    return is;
}

std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<std::string, std::string>>& p)
{
    for (auto it = p.begin(); it != p.end(); it++)
    {
        os << it->first << ':' << it->second;

        if (it != --p.end())
        {
            os << ", ";
        }
    }
    return os;
}

void Cmdline::parse(int argc, const char** argv)
{
    cmd.parse(argc, argv);

    pluginArgs = convertPluginArgValuesToArgVectors(pluginArguments.getValue());
}

std::map<std::string, std::vector<std::string>, std::less<>>
Cmdline::convertPluginArgValuesToArgVectors(const std::vector<std::pair<std::string, std::string>>& pluginArgs)
{
    std::map<std::string, std::vector<std::string>, std::less<>> pluginArgsMap{};
    for (const auto& el : pluginArgs)
    {
        auto pluginFileName = fmt::format("lib{}.so", el.first);
        std::vector<std::string> pluginCmdline{pluginFileName};
        auto argv = splitArgs(el.second);
        pluginCmdline.insert(pluginCmdline.end(), argv.begin(), argv.end());
        auto insertResult = pluginArgsMap.insert(std::make_pair(pluginFileName, pluginCmdline));

        if (!insertResult.second)
        {
            throw std::runtime_error(fmt::format("Duplicate plugin name {}", el.first));
        }
    }
    return pluginArgsMap;
}

std::vector<std::string> Cmdline::splitArgs(const std::string& arg)
{
    std::istringstream iss{arg};
    return {std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>()};
}
