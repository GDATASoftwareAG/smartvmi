#include "LibvmiInterface.h"
#include "../GlobalControl.h"
#include "../io/grpc/GRPCLogger.h"
#include "../os/PagingDefinitions.h"
#include "VmiInitData.h"
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
        libvmiInterfaceInstance->logger->warning("Failed to clear event",
                                                 {logfield::create("eventAddress", Convenience::intToHex(event)),
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
        throw VmiException(std::string(__func__) + ": Unable to register cr3 event.");
    }
    if (cr3Trigger)
    {
        throw std::runtime_error(std::string(__func__) +
                                 ": Initial value of cr3Trigger is 'true'. This should not occur.");
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
        throw VmiException(std::string(__func__) + ": No cr3 event caught after 5 tries.");
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
        throw VmiException(std::string(__func__) + ": Unable to clear cr3 event.");
    }
    cr3EventHandler();
}

void LibvmiInterface::clearEvent(vmi_event_t& event, bool deallocate)
{
    if (vmi_clear_event(vmiInstance, &event, deallocate ? &LibvmiInterface::freeEvent : nullptr) != VMI_SUCCESS)
    {
        throw VmiException(std::string(__func__) + ": Unable to clear event.");
    }
}

uint8_t LibvmiInterface::read8PA(const uint64_t physicalAddress)
{
    uint8_t extractedValue;
    auto accessContext = createPhysicalAddressAccessContext(physicalAddress);
    std::lock_guard<std::mutex> lock(libvmiLock);
    if (vmi_read_8(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
    {
        throw VmiException(std::string(__func__) + ": Unable to read one byte from PA " +
                           Convenience::intToHex(physicalAddress));
    }
    return extractedValue;
}

uint32_t LibvmiInterface::read32VA(const uint64_t virtualAddress, const uint64_t cr3)
{
    uint32_t extractedValue;
    auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
    std::lock_guard<std::mutex> lock(libvmiLock);
    if (vmi_read_32(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
    {
        throw VmiException(std::string(__func__) + ": Unable to read 4 bytes from VA " +
                           Convenience::intToHex(virtualAddress));
    }
    return extractedValue;
}

uint64_t LibvmiInterface::read64VA(const uint64_t virtualAddress, const uint64_t cr3)
{
    uint64_t extractedValue;
    auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
    std::lock_guard<std::mutex> lock(libvmiLock);
    if (vmi_read_64(vmiInstance, &accessContext, &extractedValue) == VMI_FAILURE)
    {
        throw VmiException(std::string(__func__) + ": Unable to read 8 bytes from VA " +
                           Convenience::intToHex(virtualAddress));
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
        throw VmiException(std::string(__func__) + ": Unable to write " + Convenience::intToHex(value) + " to PA " +
                           Convenience::intToHex(physicalAddress));
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
        throw VmiException(std::string(__func__) + ": Error while waiting for vmi events.");
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
            throw std::invalid_argument(std::string(__func__) +
                                        ": Event type unknown. Type = " + std::to_string(eventType));
        }
    }
    return typeAsString;
}

void LibvmiInterface::registerEvent(vmi_event_t& event)
{
    if (vmi_register_event(vmiInstance, &event) == VMI_FAILURE)
    {
        throw VmiException(std::string(__func__) +
                           "Unable to register event with type: " + eventTypeToString(event.type));
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
    uint64_t kernelSymbolAddress;
    if (vmi_translate_ksym2v(vmiInstance, kernelSymbolName.c_str(), &kernelSymbolAddress) != VMI_SUCCESS)
    {
        throw VmiException(std::string(__func__) + ": Unable to find kernel symbol " + kernelSymbolName);
    }
    return kernelSymbolAddress;
}

uint64_t LibvmiInterface::convertVAToPA(uint64_t virtualAddress, uint64_t processCr3)
{
    uint64_t physicalAddress;
    if (vmi_pagetable_lookup(vmiInstance, processCr3, virtualAddress, &physicalAddress) != VMI_SUCCESS)
    {
        throw VmiException(std::string(__func__) + ": Conversion of address " + Convenience::intToHex(virtualAddress) +
                           " with cr3 " + Convenience::intToHex(processCr3) + " not possible.");
    }
    return physicalAddress;
}

uint64_t LibvmiInterface::convertPidToDtb(pid_t processID)
{
    uint64_t dtb;
    if (vmi_pid_to_dtb(vmiInstance, processID, &dtb) != VMI_SUCCESS)
    {
        throw VmiException("Unable to obtain the dtb for pid " + Convenience::intToDec(processID));
    }
    return dtb;
}

pid_t LibvmiInterface::convertDtbToPid(uint64_t dtb)
{
    pid_t pid;
    if (vmi_dtb_to_pid(vmiInstance, dtb, &pid) != VMI_SUCCESS)
    {
        throw VmiException("Unable obtain the pid for dtb " + Convenience::intToHex(dtb));
    }
    return pid;
}

void LibvmiInterface::pauseVm()
{
    auto status = vmi_pause_vm(vmiInstance);
    if (status != VMI_SUCCESS)
    {
        throw VmiException(std::string(__func__) + ": Unable to pause the vm");
    }
}

void LibvmiInterface::resumeVm()
{
    auto status = vmi_resume_vm(vmiInstance);
    if (status != VMI_SUCCESS)
    {
        throw VmiException(std::string(__func__) + ": Unable to resume the vm");
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
        throw VmiException(std::string(__func__) + ": Error while checking for pending events. Libvmi returned " +
                           Convenience::intToDec(areEventsPendingReturn));
    }
    if (areEventsPendingReturn > 0)
    {
        pending = true;
    }
    return pending;
}

/*
 * TODO: Replace this function with libvmi functions.
 * The new libvmi version has got convenience functions for Windows unicode string extraction as well as conversion to
 * utf-8.
 */
std::unique_ptr<std::string>
LibvmiInterface::extractWStringAtVA(const uint64_t wstringVA, const size_t sizeInBytes, const uint64_t cr3)
{
    std::unique_ptr<std::string> convertedString;
    try
    {
        auto byteBuffer = std::vector<uint8_t>(sizeInBytes);
        if (!readXVA(wstringVA, cr3, byteBuffer))
        {
            throw VmiException(std::string(__func__) + ": Unable to read " + std::to_string(sizeInBytes) +
                               " bytes from VA " + Convenience::intToHex(wstringVA) + " with cr3 " +
                               Convenience::intToHex(cr3));
        }
        /*
         * readXVA returns a byte array from memory agnostic to the respective context in memory.
         * However, Windows uses utf16 to store wide strings.
         * Since utf16 chars are 2 bytes long, the length of the string is ArrayLength / 2.
         */
        std::u16string unicodeString(reinterpret_cast<char16_t*>(byteBuffer.data()), byteBuffer.size() / 2);
        convertedString = std::make_unique<std::string>(wstringConverter.to_bytes(unicodeString));
    }
    catch (const std::range_error&)
    {
        throw VmiException(std::string(__func__) + ": Unable to convert unicode string");
    }
    return convertedString;
}

std::unique_ptr<std::string> LibvmiInterface::extractStringAtVA(const uint64_t virtualAddress, const uint64_t cr3)
{
    auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
    std::lock_guard<std::mutex> lock(libvmiLock);
    auto rawString = vmi_read_str(vmiInstance, &accessContext);
    if (rawString == nullptr)
    {
        throw VmiException(std::string(__func__) + ": Unable to read string at VA " +
                           Convenience::intToHex(virtualAddress));
    }
    auto result = std::make_unique<std::string>(rawString);
    free(rawString);
    return result;
}

void LibvmiInterface::stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId)
{
    if (vmi_stop_single_step_vcpu(vmiInstance, event, vcpuId) != VMI_SUCCESS)
    {
        throw VmiException("Failed to stop single stepping for vcpu " + Convenience::intToDec(vcpuId));
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
