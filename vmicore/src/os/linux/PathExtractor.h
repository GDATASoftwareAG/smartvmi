#ifndef VMICORE_LINUX_PATHEXTRACTION_H
#define VMICORE_LINUX_PATHEXTRACTION_H

#include "../../vmi/LibvmiInterface.h"
#include <cstdint>
#include <memory>
#include <string>

namespace VmiCore::Linux
{
    class PathExtractor
    {
      public:
        explicit PathExtractor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                               const std::shared_ptr<ILogging>& logging);

        [[nodiscard]] std::string extractDPath(uint64_t path) const;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        std::unique_ptr<ILogger> logger;

        [[nodiscard]] std::string createPath(uint64_t dentry, uint64_t mnt) const;
    };
}
#endif // VMICORE_LINUX_PATHEXTRACTION_H
