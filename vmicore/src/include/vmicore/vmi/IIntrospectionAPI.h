#ifndef VMICORE_IINTROSPECTIONAPI_H
#define VMICORE_IINTROSPECTIONAPI_H

#include "../os/OperatingSystem.h"
#include "../types.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace VmiCore
{
    /**
     * A thin, thread-safe wrapper around all low level introspection functionality that is deemed safe to access
     * without interfering with VMICore or other plugins.
     */
    class IIntrospectionAPI
    {
      public:
        virtual ~IIntrospectionAPI() = default;

        [[nodiscard]] virtual uint8_t read8PA(addr_t pyhsicalAddress) = 0;

        [[nodiscard]] virtual uint64_t read64PA(addr_t physicalAddress) = 0;

        [[nodiscard]] virtual uint8_t read8VA(addr_t virtualAddress, addr_t cr3) = 0;

        [[nodiscard]] virtual uint32_t read32VA(addr_t virtualAddress, addr_t cr3) = 0;

        [[nodiscard]] virtual uint64_t read64VA(addr_t virtualAddress, addr_t cr3) = 0;

        [[nodiscard]] virtual uint64_t readVA(addr_t virtualAddress, addr_t dtb, std::size_t size) = 0;

        [[nodiscard]] virtual bool
        readXVA(addr_t virtualAddress, addr_t cr3, std::vector<uint8_t>& content, std::size_t size) = 0;

        [[nodiscard]] virtual uint64_t getCurrentVmId() = 0;

        [[nodiscard]] virtual uint getNumberOfVCPUs() const = 0;

        [[nodiscard]] virtual addr_t translateKernelSymbolToVA(const std::string& kernelSymbolName) = 0;

        [[nodiscard]] virtual addr_t
        translateUserlandSymbolToVA(addr_t moduleBaseAddress, addr_t dtb, const std::string& userlandSymbolName) = 0;

        [[nodiscard]] virtual addr_t convertVAToPA(addr_t virtualAddress, addr_t cr3Register) = 0;

        [[nodiscard]] virtual addr_t convertPidToDtb(pid_t processID) = 0;

        [[nodiscard]] virtual pid_t convertDtbToPid(addr_t dtb) = 0;

        [[nodiscard]] virtual std::optional<std::string> extractWStringAtVA(addr_t stringVA, addr_t cr3) = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string> extractUnicodeStringAtVA(addr_t stringVA, addr_t cr3) = 0;

        [[nodiscard]] virtual std::optional<std::unique_ptr<std::string>> tryExtractUnicodeStringAtVA(addr_t stringVA,
                                                                                                      addr_t cr3) = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string> extractStringAtVA(addr_t virtualAddress, addr_t cr3) = 0;

        [[nodiscard]] virtual OperatingSystem getOsType() = 0;

        [[nodiscard]] virtual uint16_t getWindowsBuild() = 0;

        [[nodiscard]] virtual addr_t getOffset(const std::string& name) = 0;

        [[nodiscard]] virtual addr_t getKernelStructOffset(const std::string& structName,
                                                           const std::string& member) = 0;

        [[nodiscard]] virtual std::size_t getStructSizeFromJson(const std::string& struct_name) = 0;

        [[nodiscard]] virtual bool isInitialized() const = 0;

        [[nodiscard]] virtual std::tuple<addr_t, std::size_t, std::size_t>
        getBitfieldOffsetAndSizeFromJson(const std::string& struct_name, const std::string& struct_member) = 0;

        virtual void flushV2PCache(addr_t pt) = 0;

        virtual void flushPageCache() = 0;

      protected:
        IIntrospectionAPI() = default;
    };
}

#endif // VMICORE_IINTROSPECTIONAPI_H
