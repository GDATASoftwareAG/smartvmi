#ifndef VMICORE_LEGACYLOGGING_H
#define VMICORE_LEGACYLOGGING_H

#include "../../config/IConfigParser.h"
#include "../IFileTransport.h"
#include <fstream>
#include <map>
#include <memory>

namespace VmiCore
{
    class LegacyLogging : public IFileTransport
    {
      public:
        explicit LegacyLogging(std::shared_ptr<IConfigParser> configInterface);
        ~LegacyLogging() override = default;

        void saveBinaryToFile(const std::string_view& logFileName, const std::vector<uint8_t>& data) override;

      private:
        std::shared_ptr<IConfigParser> configInterface;
    };
}

#endif // VMICORE_LEGACYLOGGING_H
