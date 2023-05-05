#ifndef VMICORE_LIBVMIINTERFACE_H
#define VMICORE_LIBVMIINTERFACE_H

#include "../config/IConfigParser.h"
#include "../io/IEventStream.h"
#include "../io/ILogging.h"
#include <fmt/core.h>
#include <libvmi/events.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <vmicore/io/ILogger.h>
#include <vmicore/os/OperatingSystem.h>
#include <vmicore/types.h>
#include <vmicore/vmi/IIntrospectionAPI.h>

#define LIBVMI_EXTRA_JSON

#include "libvmi/libvmi_extra.h"
#include <json-c/json.h>

namespace VmiCore
{
    class ILibvmiInterface : public IIntrospectionAPI
    {
      public:
        constexpr static addr_t flushAllPTs = ~0ull;

        ~ILibvmiInterface() override = default;

        virtual void initializeVmi() = 0;

        virtual void clearEvent(vmi_event_t& event, bool deallocate) = 0;

        virtual void write8PA(addr_t physicalAddress, uint8_t value) = 0;

        virtual void eventsListen(uint32_t timeout) = 0;

        virtual void registerEvent(vmi_event_t& event) = 0;

        virtual void pauseVm() = 0;

        virtual void resumeVm() = 0;

        virtual bool areEventsPending() = 0;

        virtual void stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId) = 0;

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

        void initializeVmi() override;

        void clearEvent(vmi_event_t& event, bool deallocate) override;

        uint8_t read8PA(addr_t pyhsicalAddress) override;

        uint64_t read64PA(const addr_t physicalAddress) override;

        uint8_t read8VA(addr_t virtualAddress, addr_t cr3) override;

        uint32_t read32VA(addr_t virtualAddress, addr_t cr3) override;

        uint64_t read64VA(addr_t virtualAddress, addr_t cr3) override;

        bool readXVA(const addr_t virtualAddress,
                     const addr_t cr3,
                     std::vector<uint8_t>& content,
                     std::size_t size) override;

        void write8PA(addr_t physicalAddress, uint8_t value) override;

        void eventsListen(uint32_t timeout) override;

        void registerEvent(vmi_event_t& event) override;

        [[nodiscard]] uint64_t getCurrentVmId() override;

        [[nodiscard]] uint getNumberOfVCPUs() const override;

        addr_t translateKernelSymbolToVA(const std::string& kernelSymbolName) override;

        addr_t translateUserlandSymbolToVA(addr_t moduleBaseAddress,
                                           addr_t dtb,
                                           const std::string& userlandSymbolName) override;

        [[nodiscard]] addr_t convertVAToPA(addr_t virtualAddress, addr_t processCr3) override;

        [[nodiscard]] addr_t convertPidToDtb(pid_t processID) override;

        [[nodiscard]] pid_t convertDtbToPid(addr_t dtb) override;

        void pauseVm() override;

        void resumeVm() override;

        [[nodiscard]] bool areEventsPending() override;

        [[nodiscard]] std::unique_ptr<std::string> extractUnicodeStringAtVA(addr_t stringVA, addr_t cr3) override;

        [[nodiscard]] std::optional<std::string> extractWStringAtVA(const addr_t stringVA, const addr_t cr3) override;

        [[nodiscard]] std::optional<std::unique_ptr<std::string>>
        tryExtractUnicodeStringAtVA(const addr_t stringVA, const addr_t cr3) override;

        [[nodiscard]] std::unique_ptr<std::string> extractStringAtVA(addr_t virtualAddress, addr_t cr3) override;

        void stopSingleStepForVcpu(vmi_event_t* event, uint vcpuId) override;

        [[nodiscard]] OperatingSystem getOsType() override;

        template <typename T> std::unique_ptr<T> readVa(const addr_t virtualAddress, const addr_t cr3)
        {
            auto accessContext = createVirtualAddressAccessContext(virtualAddress, cr3);
            auto exctractedValue = std::make_unique<T>();
            std::lock_guard<std::mutex> lock(libvmiLock);
            if (vmi_read(vmiInstance, &accessContext, sizeof(T), exctractedValue.get(), nullptr) != VMI_SUCCESS)
            {
                throw VmiException(fmt::format("{}: Unable to read {} bytes from VA {:#x} with cr3 {:#x}",
                                               __func__,
                                               sizeof(T),
                                               accessContext.addr,
                                               accessContext.pt));
            }
            return exctractedValue;
        }

        [[nodiscard]] addr_t getOffset(const std::string& name) override;

        [[nodiscard]] addr_t getKernelStructOffset(const std::string& structName, const std::string& member) override;

        [[nodiscard]] std::size_t getStructSizeFromJson(const std::string& struct_name) override;

        [[nodiscard]] bool isInitialized() const override;

        [[nodiscard]] std::tuple<addr_t, std::size_t, std::size_t>
        getBitfieldOffsetAndSizeFromJson(const std::string& structName, const std::string& structMember) override;

      private:
        uint numberOfVCPUs{};
        std::shared_ptr<IConfigParser> configInterface;
        std::unique_ptr<ILogger> logger;
        std::shared_ptr<IEventStream> eventStream;
        vmi_instance_t vmiInstance{};
        std::mutex libvmiLock{};
        std::mutex eventsListenLock{};

        static std::unique_ptr<std::string> createConfigString(const std::string& offsetsFile);

        static void freeEvent(vmi_event_t* event, status_t rc);

        static access_context_t createPhysicalAddressAccessContext(addr_t physicalAddress);

        static access_context_t createVirtualAddressAccessContext(addr_t virtualAddress, addr_t cr3);

        void flushV2PCache(addr_t pt) override;

        void flushPageCache() override;
    };
}

#endif // VMICORE_LIBVMIINTERFACE_H
