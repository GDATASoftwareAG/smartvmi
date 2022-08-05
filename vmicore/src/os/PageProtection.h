#ifndef VMICORE_PAGEPROTECTION_H
#define VMICORE_PAGEPROTECTION_H

#include "OperatingSystem.h"
#include <cstdint>
#include <string>
#include <variant>

struct ProtectionValues
{
    uint32_t readable : 1 = 0;
    uint32_t writeable : 1 = 0;
    uint32_t executable : 1 = 0;
    uint32_t copyOnWrite : 1 = 0;
};

class PageProtection
{
  public:
    PageProtection() = default;

    PageProtection(uint32_t value, OperatingSystem os);

    [[nodiscard]] ProtectionValues get() const;

    [[nodiscard]] uint64_t getRaw() const;

    [[nodiscard]] std::string toString() const;

  private:
    ProtectionValues protection{};
    uint32_t raw = 0;
    OperatingSystem os = OperatingSystem::INVALID;
};

#endif // VMICORE_PAGEPROTECTION_H
