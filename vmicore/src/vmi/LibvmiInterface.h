#ifndef VMICORE_LIBVMIINTERFACE_H
#define VMICORE_LIBVMIINTERFACE_H

#include "../Convenience.h"
#include "../config/IConfigParser.h"
#include "../io/IEventStream.h"
#include "../io/ILogging.h"
#include "VmiException.h"
#include "VmiInitError.h"
#include <codecvt>
#include <functional>
#include <libvmi/events.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class ILibvmiInterface
{
  public:
    virtual ~ILibvmiInterface() = default;

    virtual void initializeVmi(const std::function<void()>& postInitializationFunction) = 0;

    virtual void waitForCR3Event(const std::function<void()>& cr3EventHandler) = 0;

    virtual void clearEvent(vmi_event_t& event, bool deallocate) = 0;

    virtual uint8_t read8PA(uint64_t pyhsicalAddress) = 0;

    virtual uint32_t read32VA(uint64_t virtualAddress, uint64_t cr3) = 0;

    virtual uint64_t read64VA(uint64_t virtualAddress, uint64_t cr3) = 0;

    virtual bool readXVA(uint64_t virtualAddress, uint64_t cr3, std::vector<uint8_t>& content) = 0;

    virtual void write8PA(uint64_t physicalAddress, uint8_t value) = 0;

    virtual void waitForEvent() = 0;

    virtual void registerEvent(vmi_event_t& event) = 0;

    virtual uint64_t getCurrentVmId() = 0;

    virtual uint getNumberOfVCPUs() = 0;

    virtual uint64_t translateKernelSymbolToVA(const std::string& kernelSymbolName) = 0;

    virtual uint64_t convertVAToPA(uint64_t virtualAddress, uint64_t cr3Register) = 0;

    virtual uint64_t convertPidToDtb(pid_t processID) = 0;

    virtual pid_t convertDtbToPid(uint64_t dtb) = 0;

    virtual void pauseVm() = 0;

    virtual void resumeVm() = 0;

    virtual bool isVmAlive() = 0;

    virtual bool areEventsPending() = 0;

    virtual std::unique_ptr<std::string> extractWStringAtVA(uint64_t wstringVA, size_t sizeInBytes, uint64_t cr3) = 0;

    virtual std::unique_ptr<std::string> extractStringAtVA(uint64_t virtualAddress, uint64_t cr3) = 0;

    virtual void stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId) = 0;

    virtual uint64_t getSystemCr3() = 0;

  protected:
    ILibvmiInterface() = default;
};

class LibvmiInterface : public ILibvmiInterface
{
  public:
    LibvmiInterface(std::shared_ptr<IConfigParser> configInterface,
                    std::shared_ptr<ILogging> loggingLib,
                    std::shared_ptr<IEventStream> eventStream);

    ~LibvmiInterface() override;

    void initializeVmi(const std::function<void()>& postInitializationFunction) override;

    void waitForCR3Event(const std::function<void()>& cr3EventHandler) override;

    void clearEvent(vmi_event_t& event, bool deallocate) override;

    uint8_t read8PA(uint64_t pyhsicalAddress) override;

    uint32_t read32VA(uint64_t virtualAddress, uint64_t cr3) override;

    uint64_t read64VA(uint64_t virtualAddress, uint64_t cr3) override;

    bool readXVA(uint64_t virtualAddress, uint64_t cr3, std::vector<uint8_t>& content) override;

    void write8PA(uint64_t physicalAddress, uint8_t value) override;

    void waitForEvent() override;

    void registerEvent(vmi_event_t& event) override;

    uint64_t getCurrentVmId() override;

    uint getNumberOfVCPUs() override;

    uint64_t translateKernelSymbolToVA(const std::string& kernelSymbolName) override;

    uint64_t convertVAToPA(uint64_t virtualAddress, uint64_t processCr3) override;

    uint64_t convertPidToDtb(pid_t processID) override;

    pid_t convertDtbToPid(uint64_t dtb) override;

    void pauseVm() override;

    void resumeVm() override;

    bool isVmAlive() override;

    bool areEventsPending() override;

    std::unique_ptr<std::string> extractWStringAtVA(uint64_t wstringVA, size_t sizeInBytes, uint64_t cr3) override;

    std::unique_ptr<std::string> extractStringAtVA(uint64_t virtualAddress, uint64_t cr3) override;

    static event_response_t _cr3Callback(vmi_instance_t vmi, vmi_event_t* event);

    void stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId) override;

    uint64_t getSystemCr3() override;

    template <typename T> std::unique_ptr<T> readVa(const uint64_t virtualAddress, const uint64_t cr3)
    {
        auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
        auto exctractedValue = std::make_unique<T>();
        std::lock_guard<std::mutex> lock(libvmiLock);
        if (vmi_read(vmiInstance, &accessContext, sizeof(T), exctractedValue.get(), nullptr) != VMI_SUCCESS)
        {
            throw VmiException(std::string(__func__) + ": Unable to read " + std::to_string(sizeof(T)) +
                               " bytes from VA " + Convenience::intToHex(accessContext.addr) + " with cr3 " +
                               Convenience::intToHex(accessContext.pt));
        }
        return exctractedValue;
    }

  private:
    uint numberOfVCPUs{};
    std::shared_ptr<IConfigParser> configInterface;
    std::function<void()> cr3EventHandler;
    std::unique_ptr<ILogger> logger;
    std::shared_ptr<IEventStream> eventStream;
    vmi_instance_t vmiInstance{};
    std::mutex libvmiLock{};
    /*
     * TODO: As of C++17 codecvt is deprecated. Wait until a better solution is standardized, then replace codecvt:
     * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0618r0.html
     */
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> wstringConverter;
    uint64_t systemCr3{};

    static void freeEvent(vmi_event_t* event, status_t rc);

    void waitForCR3Event();

    void cr3Callback(vmi_event_t* event);

    static access_context_t createPhysicalAddressAccessContext(uint64_t physicalAddress);

    static access_context_t createVirtualAddressAccessContext(uint64_t virtualAddress, uint64_t cr3);

    static std::unique_ptr<std::string> createConfigString(const std::string& offsetsFile);
};

#endif // VMICORE_LIBVMIINTERFACE_H
