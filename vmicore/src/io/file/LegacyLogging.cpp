#include "LegacyLogging.h"
#include <cstdint>
#include <exception>
#include <filesystem>

namespace VmiCore
{
    LegacyLogging::LegacyLogging(std::shared_ptr<IConfigParser> configInterface)
        : configInterface(std::move(configInterface))
    {
    }

    void LegacyLogging::saveBinaryToFile(const std::string& logFileName, const std::vector<uint8_t>& data)
    {
        std::filesystem::create_directories(configInterface->getResultsDirectory());
        std::ofstream ofStream;
        ofStream.exceptions(std::ios::failbit | std::ios::badbit);
        ofStream.open(configInterface->getResultsDirectory() / logFileName, std::ios::app | std::ios::binary);
        try
        {
            ofStream.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        catch (const std::exception& e)
        {
            ofStream.close();
            throw;
        }
        ofStream.close();
    }
}
