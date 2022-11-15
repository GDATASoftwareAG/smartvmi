#include "LibvmiInterface.h"
#include "../GlobalControl.h"
#include "VmiException.h"
#include "VmiInitData.h"
#include "VmiInitError.h"
#include <utility>
#include <vmicore/filename.h>
#include <vmicore/os/PagingDefinitions.h>

namespace VmiCore
{
    namespace
    {
        LibvmiInterface* libvmiInterfaceInstance = nullptr;
    }

    LibvmiInterface::LibvmiInterface(std::shared_ptr<IConfigParser> configInterface,
                                     std::shared_ptr<ILogging> loggingLib,
                                     std::shared_ptr<IEventStream> eventStream)
        : configInterface(std::move(configInterface)),
          logger(loggingLib->newNamedLogger(FILENAME_STEM)),
          eventStream(std::move(eventStream))
    {
        if (libvmiInterfaceInstance != nullptr)
        {
            throw std::runtime_error("Not allowed to initialize more than one instance of LibvmiInterface.");
        }
        libvmiInterfaceInstance = this;
    }

    LibvmiInterface::~LibvmiInterface()
    {
        vmi_resume_vm(vmiInstance);
        vmi_destroy(vmiInstance);
        libvmiInterfaceInstance = nullptr;
    }

    void LibvmiInterface::initializeVmi()
    {
        logger->info("Initialize libvmi", {logfield::create("domain", configInterface->getVmName())});

        auto configString = createConfigString(configInterface->getOffsetsFile());
        auto initData = VmiInitData(configInterface->getSocketPath());
        vmi_init_error initError;

        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_init_complete(&vmiInstance,
                              reinterpret_cast<const void*>(configInterface->getVmName().c_str()),
                              VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS,
                              initData.data,
                              VMI_CONFIG_STRING,
                              reinterpret_cast<void*>(const_cast<char*>(configString->c_str())),
                              &initError) == VMI_FAILURE)
        {
            throw VmiInitError(initError);
        }

        numberOfVCPUs = vmi_get_num_vcpus(vmiInstance);
    }

    std::unique_ptr<std::string> LibvmiInterface::createConfigString(const std::string& offsetsFile)
    {
        return std::make_unique<std::string>(R"({ ostype = "Windows"; volatility_ist = ")" + offsetsFile + R"("; })");
    }

    void LibvmiInterface::freeEvent(vmi_event_t* event, status_t rc)
    {
        if (rc != VMI_SUCCESS)
        {
            libvmiInterfaceInstance->logger->warning(
                "Failed to clear event",
                {logfield::create("eventAddress", fmt::format("{:#x}", static_cast<uint64_t>(event->type))),
                 logfield::create("type", static_cast<uint64_t>(event->type))});
        }
        free(event);
    }

    void LibvmiInterface::clearEvent(vmi_event_t& event, bool deallocate)
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_clear_event(vmiInstance, &event, deallocate ? &LibvmiInterface::freeEvent : nullptr) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to clear event.", __func__));
        }
    }

    uint8_t LibvmiInterface::read8PA(const addr_t physicalAddress)
    {
        uint8_t extractedValue = 0;
        auto accessContext = createPhysicalAddressAccessContext(physicalAddress);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read_8(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to read one byte from PA: {:#x}", __func__, physicalAddress));
        }
        return extractedValue;
    }

    uint64_t LibvmiInterface::read64PA(const addr_t physicalAddress)
    {
        uint64_t extractedValue = 0;
        auto accessContext = createPhysicalAddressAccessContext(physicalAddress);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read_64(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to read 8 bytes from PA: {:#x}", __func__, physicalAddress));
        }
        return extractedValue;
    }

    uint8_t LibvmiInterface::read8VA(const addr_t virtualAddress, const addr_t cr3)
    {
        uint8_t extractedValue = 0;
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read_8(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to read one byte from VA: {:#x}", __func__, virtualAddress));
        }
        return extractedValue;
    }

    uint32_t LibvmiInterface::read32VA(const addr_t virtualAddress, const addr_t cr3)
    {
        uint32_t extractedValue = 0;
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read_32(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to read 4 bytes from VA {:#x}", __func__, virtualAddress));
        }
        return extractedValue;
    }

    uint64_t LibvmiInterface::read64VA(const addr_t virtualAddress, const addr_t cr3)
    {
        uint64_t extractedValue = 0;
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read_64(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to read 8 bytes from VA {:#x}", __func__, virtualAddress));
        }
        return extractedValue;
    }

    bool LibvmiInterface::readXVA(const addr_t virtualAddress, const addr_t cr3, std::vector<uint8_t>& content)
    {
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read(vmiInstance, &accessContext, content.size(), content.data(), nullptr) != VMI_SUCCESS)
        {
            return false;
        }
        return true;
    }

    std::vector<void*> LibvmiInterface::mmapGuest(addr_t baseVA, addr_t dtb, std::size_t numberOfPages)
    {
        auto accessPointers = std::vector<void*>(numberOfPages);
        auto accessContext = createVirtualAddressAccessContext(baseVA, dtb);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_mmap_guest(vmiInstance, &accessContext, numberOfPages, accessPointers.data()) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to create memory mapping for VA {:#x} with number of pages {}",
                                           __func__,
                                           baseVA,
                                           numberOfPages));
        }
        return accessPointers;
    }

    void LibvmiInterface::write8PA(const addr_t physicalAddress, uint8_t value)
    {
        auto accessContext = createPhysicalAddressAccessContext(physicalAddress);
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_write_8(vmiInstance, &accessContext, &value) == VMI_FAILURE)
        {
            throw VmiException(fmt::format("{}: Unable to write {:#x} to PA {:#x}", __func__, value, physicalAddress));
        }
    }

    access_context_t LibvmiInterface::createPhysicalAddressAccessContext(addr_t physicalAddress)
    {
        access_context_t accessContext{};
        accessContext.version = ACCESS_CONTEXT_VERSION;
        accessContext.translate_mechanism = VMI_TM_NONE;
        accessContext.addr = physicalAddress;
        return accessContext;
    }

    access_context_t LibvmiInterface::createVirtualAddressAccessContext(addr_t virtualAddress, addr_t cr3)
    {
        access_context_t accessContext{};
        accessContext.version = ACCESS_CONTEXT_VERSION;
        accessContext.translate_mechanism = VMI_TM_PROCESS_PT;
        accessContext.addr = virtualAddress;
        accessContext.page_table = cr3;
        return accessContext;
    }

    void LibvmiInterface::eventsListen(uint32_t timeout)
    {
        std::lock_guard<std::mutex> lock(eventsListenLock);
        auto status = vmi_events_listen(vmiInstance, timeout);
        if (status != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Error while waiting for vmi events.", __func__));
        }
    }

    std::string eventTypeToString(vmi_event_type_t eventType)
    {
        std::string typeAsString;
        switch (eventType)
        {
            case VMI_EVENT_INTERRUPT:
            {
                typeAsString = "Interrupt Event";
                break;
            }
            case VMI_EVENT_MEMORY:
            {
                typeAsString = "Memory Event";
                break;
            }
            default:
            {
                throw std::invalid_argument(
                    fmt::format("{}: Event type unknown. Type = {}", __func__, std::to_string(eventType)));
            }
        }
        return typeAsString;
    }

    void LibvmiInterface::registerEvent(vmi_event_t& event)
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_register_event(vmiInstance, &event) == VMI_FAILURE)
        {
            throw VmiException(
                fmt::format("{}: Unable to register event with type: {}", __func__, eventTypeToString(event.type)));
        }
    }

    uint64_t LibvmiInterface::getCurrentVmId()
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        return vmi_get_vmid(vmiInstance);
    }

    uint LibvmiInterface::getNumberOfVCPUs() const
    {
        return numberOfVCPUs;
    }

    addr_t LibvmiInterface::translateKernelSymbolToVA(const std::string& kernelSymbolName)
    {
        addr_t kernelSymbolAddress = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_translate_ksym2v(vmiInstance, kernelSymbolName.c_str(), &kernelSymbolAddress) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to find kernel symbol {}", __func__, kernelSymbolName));
        }
        return kernelSymbolAddress;
    }

    addr_t LibvmiInterface::translateUserlandSymbolToVA(addr_t moduleBaseAddress,
                                                        addr_t dtb,
                                                        const std::string& userlandSymbolName)
    {
        auto ctx = createVirtualAddressAccessContext(moduleBaseAddress, dtb);
        addr_t userlandSymbolVA = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_translate_sym2v(vmiInstance, &ctx, userlandSymbolName.c_str(), &userlandSymbolVA) != VMI_SUCCESS)
        {
            throw VmiException(
                fmt::format("{}: Unable to get address of userland symbol {} for VA {} with pid {} and dtb {}",
                            __func__,
                            userlandSymbolName,
                            moduleBaseAddress,
                            dtb,
                            dtb));
        }

        return userlandSymbolVA;
    }

    addr_t LibvmiInterface::convertVAToPA(addr_t virtualAddress, addr_t processCr3)
    {
        addr_t physicalAddress = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_pagetable_lookup(vmiInstance, processCr3, virtualAddress, &physicalAddress) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format(
                "{}: Conversion of address {:#x} with cr3 {:#x} not possible.", __func__, virtualAddress, processCr3));
        }
        return physicalAddress;
    }

    addr_t LibvmiInterface::convertPidToDtb(pid_t processID)
    {
        addr_t dtb = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_pid_to_dtb(vmiInstance, processID, &dtb) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("Unable to obtain the dtb for pid {}", processID));
        }
        return dtb;
    }

    pid_t LibvmiInterface::convertDtbToPid(addr_t dtb)
    {
        vmi_pid_t pid = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_dtb_to_pid(vmiInstance, dtb, &pid) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("Unable obtain the pid for dtb {:#x}", dtb));
        }
        return pid;
    }

    void LibvmiInterface::pauseVm()
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        auto status = vmi_pause_vm(vmiInstance);
        if (status != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to pause the vm", __func__));
        }
    }

    void LibvmiInterface::resumeVm()
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        auto status = vmi_resume_vm(vmiInstance);
        if (status != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to resume the vm", __func__));
        }
    }

    bool LibvmiInterface::areEventsPending()
    {
        bool pending = false;
        std::lock_guard<std::mutex> lock(libvmiLock);
        auto areEventsPendingReturn = vmi_are_events_pending(vmiInstance);
        if (areEventsPendingReturn == -1)
        {
            throw VmiException(fmt::format(
                "{}: Error while checking for pending events. Libvmi returned {}", __func__, areEventsPendingReturn));
        }
        if (areEventsPendingReturn > 0)
        {
            pending = true;
        }
        return pending;
    }

    std::unique_ptr<std::string> LibvmiInterface::extractUnicodeStringAtVA(const addr_t stringVA, const addr_t cr3)
    {
        auto accessContext = createVirtualAddressAccessContext(stringVA, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        auto* extractedUnicodeString = vmi_read_unicode_str(vmiInstance, &accessContext);
        auto convertedUnicodeString = unicode_string_t{};
        auto success = vmi_convert_str_encoding(extractedUnicodeString, &convertedUnicodeString, "UTF-8");
        vmi_free_unicode_str(extractedUnicodeString);
        if (success != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to convert unicode string", __func__));
        }
        auto result = std::make_unique<std::string>(reinterpret_cast<char*>(convertedUnicodeString.contents),
                                                    convertedUnicodeString.length);
        free(convertedUnicodeString.contents); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        return result;
    }

    std::unique_ptr<std::string> LibvmiInterface::extractStringAtVA(const addr_t virtualAddress, const addr_t cr3)
    {
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        std::lock_guard<std::mutex> lock(libvmiLock);
        auto* rawString = vmi_read_str(vmiInstance, &accessContext);
        if (rawString == nullptr)
        {
            throw VmiException(fmt::format("{}: Unable to read string at VA {:#x}", __func__, virtualAddress));
        }
        auto result = std::make_unique<std::string>(rawString);
        free(rawString);
        return result;
    }

    void LibvmiInterface::stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId)
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_stop_single_step_vcpu(vmiInstance, event, vcpuId) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("Failed to stop single stepping for vcpu {}", vcpuId));
        }
    }

    OperatingSystem LibvmiInterface::getOsType()
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        switch (vmi_get_ostype(vmiInstance))
        {
            case VMI_OS_LINUX:
                return OperatingSystem::LINUX;
            case VMI_OS_WINDOWS:
                return OperatingSystem::WINDOWS;
            default:
                return OperatingSystem::INVALID;
        }
    }

    addr_t LibvmiInterface::getOffset(const std::string& name)
    {
        addr_t offset = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_get_offset(vmiInstance, name.c_str(), &offset) != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to find offset {}", __func__, name));
        }
        return offset;
    }

    addr_t LibvmiInterface::getKernelStructOffset(const std::string& structName, const std::string& member)
    {
        addr_t memberAddress = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_get_kernel_struct_offset(vmiInstance, structName.c_str(), member.c_str(), &memberAddress) !=
            VMI_SUCCESS)
        {
            throw VmiException(
                fmt::format("Failed to get offset of kernel struct {} with member {}", structName, member));
        }
        return memberAddress;
    }

    size_t LibvmiInterface::getStructSizeFromJson(const std::string& struct_name)
    {
        size_t size = 0;
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_get_struct_size_from_json(vmiInstance, vmi_get_kernel_json(vmiInstance), struct_name.c_str(), &size) !=
            VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable to extract struct size of {}", __func__, struct_name));
        }
        return size;
    }

    bool LibvmiInterface::isInitialized() const
    {
        return vmiInstance != nullptr;
    }

    std::tuple<addr_t, size_t, size_t>
    LibvmiInterface::getBitfieldOffsetAndSizeFromJson(const std::string& structName, const std::string& structMember)
    {
        addr_t offset{};
        size_t startBit{};
        size_t endBit{};

        std::lock_guard<std::mutex> lock(libvmiLock);
        auto ret = vmi_get_bitfield_offset_and_size_from_json(vmiInstance,
                                                              vmi_get_kernel_json(vmiInstance),
                                                              structName.c_str(),
                                                              structMember.c_str(),
                                                              &offset,
                                                              &startBit,
                                                              &endBit);
        if (ret != VMI_SUCCESS)
        {
            throw VmiException(fmt::format("{}: Unable extract offset and size from struct {} with member {}",
                                           __func__,
                                           structName,
                                           structMember));
        }
        return std::make_tuple(offset, startBit, endBit);
    }

    void LibvmiInterface::flushV2PCache(addr_t pt)
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        vmi_v2pcache_flush(vmiInstance, pt);
    }

    void LibvmiInterface::flushPageCache()
    {
        std::lock_guard<std::mutex> lock(libvmiLock);
        vmi_pagecache_flush(vmiInstance);
    }
}
