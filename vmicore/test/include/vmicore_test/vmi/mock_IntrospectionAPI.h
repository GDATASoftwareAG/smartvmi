#ifndef VMICORE_MOCK_INTROSPECTIONAPI_H
#define VMICORE_MOCK_INTROSPECTIONAPI_H

#include <gmock/gmock.h>
#include <vmicore/vmi/IIntrospectionAPI.h>

namespace VmiCore
{
    class MockIntrospectionAPI : public IIntrospectionAPI
    {
      public:
        MOCK_METHOD(uint8_t, read8PA, (uint64_t), (override));

        MOCK_METHOD(uint64_t, read64PA, (uint64_t), (override));

        MOCK_METHOD(uint8_t, read8VA, (uint64_t, uint64_t), (override));

        MOCK_METHOD(uint32_t, read32VA, (uint64_t, uint64_t), (override));

        MOCK_METHOD(uint64_t, read64VA, (uint64_t, uint64_t), (override));

        MOCK_METHOD(uint64_t, readVA, (addr_t, addr_t, std::size_t), (override));

        MOCK_METHOD(bool, readXVA, (uint64_t, uint64_t, std::vector<uint8_t>&, std::size_t size), (override));

        MOCK_METHOD(uint64_t, getCurrentVmId, (), (override));

        MOCK_METHOD(uint, getNumberOfVCPUs, (), (const override));

        MOCK_METHOD(addr_t, translateKernelSymbolToVA, (const std::string&), (override));

        MOCK_METHOD(addr_t, translateUserlandSymbolToVA, (addr_t, addr_t, const std::string&), (override));

        MOCK_METHOD(addr_t, convertVAToPA, (addr_t, addr_t), (override));

        MOCK_METHOD(addr_t, convertPidToDtb, (pid_t), (override));

        MOCK_METHOD(pid_t, convertDtbToPid, (addr_t), (override));

        MOCK_METHOD(std::unique_ptr<std::string>, extractUnicodeStringAtVA, (addr_t, addr_t), (override));

        MOCK_METHOD(std::optional<std::string>, extractWStringAtVA, (addr_t stringVA, addr_t cr3), (override));

        MOCK_METHOD(std::optional<std::unique_ptr<std::string>>,
                    tryExtractUnicodeStringAtVA,
                    (addr_t, addr_t),
                    (override));

        MOCK_METHOD(std::unique_ptr<std::string>, extractStringAtVA, (addr_t, addr_t), (override));

        MOCK_METHOD(OperatingSystem, getOsType, (), (override));

        MOCK_METHOD(uint16_t, getWindowsBuild, (), (override));

        MOCK_METHOD(uint64_t, getOffset, (const std::string&), (override));

        MOCK_METHOD(addr_t, getKernelStructOffset, (const std::string&, const std::string&), (override));

        MOCK_METHOD(bool, isInitialized, (), (const override));

        MOCK_METHOD((std::tuple<addr_t, std::size_t, std::size_t>),
                    getBitfieldOffsetAndSizeFromJson,
                    (const std::string&, const std::string&),
                    (override));

        MOCK_METHOD(size_t, getStructSizeFromJson, (const std::string&), (override));

        MOCK_METHOD(void, flushV2PCache, (addr_t), (override));

        MOCK_METHOD(void, flushPageCache, (), (override));
    };
}

#endif // VMICORE_MOCK_INTROSPECTIONAPI_H
