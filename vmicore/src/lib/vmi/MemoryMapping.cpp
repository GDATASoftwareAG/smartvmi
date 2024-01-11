#include "MemoryMapping.h"
#include <vmicore/filename.h>

// Currently implemented in GNU libstdc++ but missing in LLVM libc++.
#ifdef __cpp_lib_is_layout_compatible
#include <type_traits>

static_assert(std::is_layout_compatible_v<mapped_region, VmiCore::MappedRegion>,
              "Layout of libvmi mapped_region not compatible with VmiCore MappedRegion");
#endif

namespace VmiCore
{
    MemoryMapping::MemoryMapping(const std::shared_ptr<ILogging>& logging,
                                 std::shared_ptr<ILibvmiInterface> vmiInterface,
                                 mapped_regions_t mappedRegions)
        : logger(logging->newNamedLogger(FILENAME_STEM)),
          vmiInterface(std::move(vmiInterface)),
          libvmiMappings(mappedRegions)
    {
    }

    MemoryMapping::~MemoryMapping()
    {
        if (isMapped)
        {
            unmap();
        }
    }

    std::span<const MappedRegion> MemoryMapping::getMappedRegions() const
    {
        if (!isMapped)
        {
            throw MemoryMappingError("Cannot retrieve mappings for regions that have already been unmapped");
        }

        return {std::bit_cast<MappedRegion*>(libvmiMappings.regions), libvmiMappings.size};
    }

    void MemoryMapping::unmap()
    {
        if (isMapped)
        {
            vmiInterface->freeMappedRegions(libvmiMappings);

            isMapped = false;
        }
    }
}
