#ifndef VMICORE_IINTROSPECTIONAPI_H
#define VMICORE_IINTROSPECTIONAPI_H

#include "../os/OperatingSystem.h"
#include "../types.h"
#include "IVmiUnicodeStruct.h"
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace VmiCore
{
    class IIntrospectionAPI
    {
      public:
        virtual ~IIntrospectionAPI() = default;

        virtual uint8_t read8PA(addr_t pyhsicalAddress) = 0;

        virtual uint64_t read64PA(const addr_t physicalAddress) = 0;

        virtual uint8_t read8VA(addr_t virtualAddress, addr_t cr3) = 0;

        virtual uint32_t read32VA(addr_t virtualAddress, addr_t cr3) = 0;

        virtual uint64_t read64VA(addr_t virtualAddress, addr_t cr3) = 0;

        virtual bool readXVA(addr_t virtualAddress, addr_t cr3, std::vector<uint8_t>& content) = 0;

        [[nodiscard]] virtual uint64_t getCurrentVmId() = 0;

        [[nodiscard]] virtual uint getNumberOfVCPUs() const = 0;

        virtual addr_t translateKernelSymbolToVA(const std::string& kernelSymbolName) = 0;

        virtual addr_t
        translateUserlandSymbolToVA(addr_t moduleBaseAddress, addr_t dtb, const std::string& userlandSymbolName) = 0;

        [[nodiscard]] virtual addr_t convertVAToPA(addr_t virtualAddress, addr_t cr3Register) = 0;

        [[nodiscard]] virtual addr_t convertPidToDtb(pid_t processID) = 0;

        [[nodiscard]] virtual pid_t convertDtbToPid(addr_t dtb) = 0;

        virtual std::unique_ptr<IVmiUnicodeStruct> extractUnicodeStringAtVA(addr_t stringVA, addr_t cr3) = 0;

        virtual std::unique_ptr<std::string> extractStringAtVA(addr_t virtualAddress, addr_t cr3) = 0;

        [[nodiscard]] virtual OperatingSystem getOsType() = 0;

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
