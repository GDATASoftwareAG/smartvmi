#ifndef VMICORE_PAGEPROTECTION_H
#define VMICORE_PAGEPROTECTION_H

#include <cstdint>
#include <string>
#include <vmicore/os/IPageProtection.h>
#include <vmicore/os/OperatingSystem.h>

namespace VmiCore
{
    class PageProtection : public IPageProtection
    {
      public:
        PageProtection() = default;

        PageProtection(uint32_t value, OperatingSystem os);

        [[nodiscard]] ProtectionValues get() const override;

        [[nodiscard]] uint64_t getRaw() const override;

        [[nodiscard]] std::string toString() const override;

      private:
        ProtectionValues protection{};
        uint32_t raw = 0;
        OperatingSystem os = OperatingSystem::INVALID;
    };
}

#endif // VMICORE_PAGEPROTECTION_H
