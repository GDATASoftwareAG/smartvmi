#include "MemoryMapping.h"
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <vmicore/filename.h>
#include <vmicore/os/PagingDefinitions.h>

namespace VmiCore
{
    MemoryMapping::MemoryMapping(addr_t guestBaseVA,
                                 const std::vector<void*>& accessPointers,
                                 const std::shared_ptr<ILogging>& logging)
        : logger(logging->newNamedLogger(FILENAME_STEM)), mappings(std::make_shared<std::vector<MappedRegion>>())
    {
        // find coherent regions that are not interrupted by NULL access pointers
        std::size_t numPagesInRegion = 0;
        void* currentBase = nullptr;

        for (std::size_t i = 0; i < accessPointers.size(); i++)
        {
            auto* accessPointer = accessPointers[i];

            if (accessPointer != nullptr)
            {
                mappingSize += PagingDefinitions::pageSizeInBytes;

                // new region starts
                if (currentBase == nullptr)
                {
                    currentBase = accessPointer;
                }
                numPagesInRegion++;
            }
            // current region ends
            else if (currentBase != nullptr)
            {
                mappings->emplace_back(guestBaseVA + (i - numPagesInRegion) * PagingDefinitions::pageSizeInBytes,
                                       std::span(reinterpret_cast<uint8_t*>(currentBase),
                                                 numPagesInRegion * PagingDefinitions::pageSizeInBytes));
                numPagesInRegion = 0;
                currentBase = nullptr;
            }
        }

        // current region is mapped until the end of the array
        if (currentBase != nullptr)
        {
            mappings->emplace_back(guestBaseVA +
                                       (accessPointers.size() - numPagesInRegion) * PagingDefinitions::pageSizeInBytes,
                                   std::span(reinterpret_cast<uint8_t*>(currentBase),
                                             numPagesInRegion * PagingDefinitions::pageSizeInBytes));
        }

        sizeInGuest = accessPointers.size() * PagingDefinitions::pageSizeInBytes;
    }

    MemoryMapping::~MemoryMapping()
    {
        if (isMapped)
        {
            unmap();
        }
    }

    std::weak_ptr<std::vector<MappedRegion>> MemoryMapping::getMappedRegions()
    {
        return mappings;
    }

    size_t MemoryMapping::getSizeInGuest()
    {
        return sizeInGuest;
    }

    void MemoryMapping::unmap()
    {
        if (!mappings->empty())
        {
            for (auto region : *mappings)
            {
                if (munmap(region.mapping.data(), region.mapping.size()) != 0)
                {
                    logger->warning("Failed to unmap guest memory",
                                    {logfield::create("pointer", reinterpret_cast<uint64_t>(region.mapping.data())),
                                     logfield::create("error", std::strerror(errno))}); // NOLINT(concurrency-mt-unsafe)
                }
            }

            isMapped = false;
        }
    }
}
