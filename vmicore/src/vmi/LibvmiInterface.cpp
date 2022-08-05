#include "LibvmiInterface.h"
#include "../GlobalControl.h"
#include "../io/grpc/GRPCLogger.h"
#include "../os/PagingDefinitions.h"
#include "VmiInitData.h"
#include <fmt/core.h>
#include <utility>

namespace
{
    LibvmiInterface* libvmiInterfaceInstance = nullptr;
    bool cr3Trigger = false;
    constexpr pid_t systemPid = 4;
}

LibvmiInterface::LibvmiInterface(std::shared_ptr<IConfigParser> configInterface,
                                 std::shared_ptr<ILogging> loggingLib,
                                 std::shared_ptr<IEventStream> eventStream)
    : configInterface(std::move(configInterface)), logger(NEW_LOGGER(loggingLib)), eventStream(std::move(eventStream))
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

void LibvmiInterface::initializeVmi(const std::function<void()>& postInitializationFunction)
{
    logger->info("Initialize libvmi", {logfield::create("domain", configInterface->getVmName())});
    logger->info("Initialize successfully initialized", {logfield::create("domain", configInterface->getVmName())});

    auto configString = createConfigString(configInterface->getOffsetsFile());
    auto initData = VmiInitData(configInterface->getSocketPath());
    vmi_init_error initError;
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
    cr3EventHandler = postInitializationFunction;
    waitForCR3Event();
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

void LibvmiInterface::waitForCR3Event()
{
    vmi_event_t cr3Event{};
    SETUP_REG_EVENT(&cr3Event, CR3, VMI_REGACCESS_W, 0, LibvmiInterface::_cr3Callback);
    cr3Event.reg_event.onchange = true;
    if (vmi_register_event(vmiInstance, &cr3Event) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to register cr3 event.", __func__));
    }
    if (cr3Trigger)
    {
        throw std::runtime_error(
            fmt::format("{}: Initial value of cr3Trigger is 'true'. This should not occur", __func__));
    }
    for (auto i = 5; i > 0; --i)
    {
        if (cr3Trigger)
        {
            break;
        }
        waitForEvent();
    }
    if (!cr3Trigger)
    {
        try
        {
            clearEvent(cr3Event, false);
        }
        catch (const VmiException& e)
        {
            logger->warning(e.what());
        }
        throw VmiException(fmt::format("{}: No cr3 event caught after 5 tries.", __func__));
    }
    else
    {
        cr3Trigger = false;
    }
}

void LibvmiInterface::waitForCR3Event(const std::function<void()>& cr3EventHandler)
{
    this->cr3EventHandler = cr3EventHandler;
    waitForCR3Event();
}

event_response_t LibvmiInterface::_cr3Callback(__attribute__((unused)) vmi_instance_t vmi, vmi_event_t* event)
{
    try
    {
        libvmiInterfaceInstance->cr3Callback(event);
    }
    catch (const std::exception& e)
    {
        GlobalControl::endVmi = true;
        libvmiInterfaceInstance->logger->error("Unexpected exception", {logfield::create("exception", e.what())});
        libvmiInterfaceInstance->eventStream->sendErrorEvent(e.what());
    }
    cr3Trigger = true;
    return VMI_EVENT_RESPONSE_NONE;
}

void LibvmiInterface::cr3Callback(vmi_event_t* event)
{
    if (vmi_clear_event(vmiInstance, event, nullptr) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to clear cr3 event.", __func__));
    }
    cr3EventHandler();
}

void LibvmiInterface::clearEvent(vmi_event_t& event, bool deallocate)
{
    if (vmi_clear_event(vmiInstance, &event, deallocate ? &LibvmiInterface::freeEvent : nullptr) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to clear event.", __func__));
    }
}

uint8_t LibvmiInterface::read8PA(const uint64_t physicalAddress)
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

uint8_t LibvmiInterface::read8VA(const uint64_t virtualAddress, const uint64_t cr3)
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

uint32_t LibvmiInterface::read32VA(const uint64_t virtualAddress, const uint64_t cr3)
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

uint64_t LibvmiInterface::read64VA(const uint64_t virtualAddress, const uint64_t cr3)
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

bool LibvmiInterface::readXVA(const uint64_t virtualAddress, const uint64_t cr3, std::vector<uint8_t>& content)
{
    auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
    std::lock_guard<std::mutex> lock(libvmiLock);
    if (vmi_read(vmiInstance, &accessContext, content.size(), content.data(), nullptr) != VMI_SUCCESS)
    {
        return false;
    }
    return true;
}

void LibvmiInterface::write8PA(const uint64_t physicalAddress, uint8_t value)
{
    auto accessContext = createPhysicalAddressAccessContext(physicalAddress);
    std::lock_guard<std::mutex> lock(libvmiLock);
    if (vmi_write_8(vmiInstance, &accessContext, &value) == VMI_FAILURE)
    {
        throw VmiException(fmt::format("{}: Unable to write {:#x} to PA {:#x}", __func__, value, physicalAddress));
    }
}

access_context_t LibvmiInterface::createPhysicalAddressAccessContext(uint64_t physicalAddress)
{
    access_context_t accessContext{};
    accessContext.version = ACCESS_CONTEXT_VERSION;
    accessContext.translate_mechanism = VMI_TM_NONE;
    accessContext.addr = physicalAddress;
    return accessContext;
}

access_context_t LibvmiInterface::createVirtualAddressAccessContext(uint64_t virtualAddress, uint64_t cr3)
{
    access_context_t accessContext{};
    accessContext.version = ACCESS_CONTEXT_VERSION;
    accessContext.translate_mechanism = VMI_TM_PROCESS_PT;
    accessContext.addr = virtualAddress;
    accessContext.page_table = cr3;
    return accessContext;
}

void LibvmiInterface::waitForEvent()
{
    auto status = vmi_events_listen(vmiInstance, 500);
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
    if (vmi_register_event(vmiInstance, &event) == VMI_FAILURE)
    {
        throw VmiException(
            fmt::format("{}: Unable to register event with type: {}", __func__, eventTypeToString(event.type)));
    }
}

uint64_t LibvmiInterface::getCurrentVmId()
{
    return (vmi_get_vmid(vmiInstance));
}

uint LibvmiInterface::getNumberOfVCPUs()
{
    return numberOfVCPUs;
}

uint64_t LibvmiInterface::translateKernelSymbolToVA(const std::string& kernelSymbolName)
{
    uint64_t kernelSymbolAddress = 0;
    if (vmi_translate_ksym2v(vmiInstance, kernelSymbolName.c_str(), &kernelSymbolAddress) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to find kernel symbol {}", __func__, kernelSymbolName));
    }
    return kernelSymbolAddress;
}

uint64_t LibvmiInterface::convertVAToPA(uint64_t virtualAddress, uint64_t processCr3)
{
    uint64_t physicalAddress = 0;
    if (vmi_pagetable_lookup(vmiInstance, processCr3, virtualAddress, &physicalAddress) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format(
            "{}: Conversion of address {:#x} with cr3 {:#x} not possible.", __func__, virtualAddress, processCr3));
    }
    return physicalAddress;
}

uint64_t LibvmiInterface::convertPidToDtb(pid_t processID)
{
    uint64_t dtb = 0;
    if (vmi_pid_to_dtb(vmiInstance, processID, &dtb) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("Unable to obtain the dtb for pid {}", processID));
    }
    return dtb;
}

pid_t LibvmiInterface::convertDtbToPid(uint64_t dtb)
{
    pid_t pid = 0;
    if (vmi_dtb_to_pid(vmiInstance, dtb, &pid) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("Unable obtain the pid for dtb {:#x}", dtb));
    }
    return pid;
}

void LibvmiInterface::pauseVm()
{
    auto status = vmi_pause_vm(vmiInstance);
    if (status != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to pause the vm", __func__));
    }
}

void LibvmiInterface::resumeVm()
{
    auto status = vmi_resume_vm(vmiInstance);
    if (status != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to resume the vm", __func__));
    }
}

bool LibvmiInterface::isVmAlive()
{
    bool isAlive = false;
    try
    {
        auto systemDtb = getSystemCr3();

        if (systemDtb != 0 && systemDtb != VMI_FAILURE)
        {
            // vmiDtbToPid does not use cached values, therefore the conversion fails if the VM is not active anymore
            if (convertDtbToPid(systemDtb) == 4)
            {
                isAlive = true;
            }
        }
    }
    catch (const VmiException& e)
    {
        logger->warning("Exception when checking if VM is alive", {logfield::create("exception", e.what())});
    }

    return isAlive;
}

bool LibvmiInterface::areEventsPending()
{
    bool pending = false;
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

std::unique_ptr<std::string> LibvmiInterface::extractUnicodeStringAtVA(const uint64_t stringVA, const uint64_t cr3)
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

std::unique_ptr<std::string> LibvmiInterface::extractStringAtVA(const uint64_t virtualAddress, const uint64_t cr3)
{
    auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
    std::lock_guard<std::mutex> lock(libvmiLock);
    auto rawString = vmi_read_str(vmiInstance, &accessContext);
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
    if (vmi_stop_single_step_vcpu(vmiInstance, event, vcpuId) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("Failed to stop single stepping for vcpu {}", vcpuId));
    }
}

uint64_t LibvmiInterface::getSystemCr3()
{
    if (systemCr3 != 0)
    {
        return systemCr3;
    }

    systemCr3 = convertPidToDtb(systemPid);
    return systemCr3;
}

addr_t LibvmiInterface::getKernelStructOffset(const std::string& structName, const std::string& member)
{
    addr_t memberAddress = 0;
    if (vmi_get_kernel_struct_offset(vmiInstance, structName.c_str(), member.c_str(), &memberAddress) != VMI_SUCCESS)
    {
        throw VmiException(fmt::format("Failed to get offset of kernel struct {} with member {}", structName, member));
    }
    return memberAddress;
}

size_t LibvmiInterface::getStructSizeFromJson(const std::string& struct_name)
{
    size_t size = 0;
    if (vmi_get_struct_size_from_json(vmiInstance, vmi_get_kernel_json(vmiInstance), struct_name.c_str(), &size) !=
        VMI_SUCCESS)
    {
        throw VmiException(fmt::format("{}: Unable to extract struct size of {}", __func__, struct_name));
    }
    return size;
}

bool LibvmiInterface::isInitialized()
{
    return vmiInstance != nullptr;
}

std::tuple<addr_t, size_t, size_t> LibvmiInterface::getBitfieldOffsetAndSizeFromJson(const std::string& structName,
                                                                                     const std::string& structMember)
{
    addr_t offset{};
    size_t startBit{};
    size_t endBit{};

    auto ret = vmi_get_bitfield_offset_and_size_from_json(vmiInstance,
                                                          vmi_get_kernel_json(vmiInstance),
                                                          structName.c_str(),
                                                          structMember.c_str(),
                                                          &offset,
                                                          &startBit,
                                                          &endBit);
    if (ret != VMI_SUCCESS)
    {
        throw VmiException(fmt::format(
            "{}: Unable extract offset and size from struct {} with member {}", __func__, structName, structMember));
    }
    return std::make_tuple(offset, startBit, endBit);
}

void LibvmiInterface::flushV2PCache(addr_t pt)
{
    vmi_v2pcache_flush(vmiInstance, pt);
}

void LibvmiInterface::flushPageCache()
{
    vmi_pagecache_flush(vmiInstance);
}
