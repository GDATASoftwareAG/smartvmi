#ifndef VMICORE_IMEMORYREGIONEXTRACTOR_H
#define VMICORE_IMEMORYREGIONEXTRACTOR_H

#include "MemoryRegion.h"
#include <list>
#include <memory>

class IMemoryRegionExtractor
{
  public:
    virtual ~IMemoryRegionExtractor() = default;

    [[nodiscard]] virtual std::unique_ptr<std::list<MemoryRegion>> extractAllMemoryRegions() const = 0;

  protected:
    IMemoryRegionExtractor() = default;
};

#endif // VMICORE_IMEMORYREGIONEXTRACTOR_H
