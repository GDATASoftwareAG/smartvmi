#ifndef VMICORE_MOCK_LIBVMIINTERFACE_H
#define VMICORE_MOCK_LIBVMIINTERFACE_H

#include <gmock/gmock.h>
#include <vmi/LibvmiInterface.h>

namespace VmiCore
{
    class MockLibvmiInterface : public ILibvmiInterface
    {
      public:
        MOCK_METHOD(void, initializeVmi, (), (override));

        MOCK_METHOD(void, clearEvent, (vmi_event_t&, bool), (override));

        MOCK_METHOD(uint8_t, read8PA, (const uint64_t), (override));

        MOCK_METHOD(uint64_t, read64PA, (const uint64_t), (override));

        MOCK_METHOD(uint8_t, read8VA, (const uint64_t, const uint64_t), (override));

        MOCK_METHOD(uint32_t, read32VA, (const uint64_t, const uint64_t), (override));

        MOCK_METHOD(uint64_t, read64VA, (const uint64_t, const uint64_t), (override));

        MOCK_METHOD(bool, readXVA, (const uint64_t, const uint64_t, std::vector<uint8_t>&), (override));

        MOCK_METHOD(std::vector<void*>, mmapGuest, (addr_t, addr_t, std::size_t), (override));

        MOCK_METHOD(void, write8PA, (const uint64_t, const uint8_t), (override));

        MOCK_METHOD(void, eventsListen, (uint32_t), (override));

        MOCK_METHOD(void, registerEvent, (vmi_event_t&), (override));

        MOCK_METHOD(uint64_t, getCurrentVmId, (), (override));

        MOCK_METHOD(uint, getNumberOfVCPUs, (), (const override));

        MOCK_METHOD(addr_t, translateKernelSymbolToVA, (const std::string&), (override));

        MOCK_METHOD(addr_t, translateUserlandSymbolToVA, (addr_t, addr_t, const std::string&), (override));

        MOCK_METHOD(addr_t, convertVAToPA, (addr_t, addr_t), (override));

        MOCK_METHOD(addr_t, convertPidToDtb, (pid_t), (override));

        MOCK_METHOD(pid_t, convertDtbToPid, (addr_t), (override));

        MOCK_METHOD(void, pauseVm, (), (override));

        MOCK_METHOD(void, resumeVm, (), (override));

        MOCK_METHOD(bool, areEventsPending, (), (override));

        MOCK_METHOD(std::unique_ptr<std::string>, extractUnicodeStringAtVA, (const addr_t, const addr_t), (override));

        MOCK_METHOD(std::unique_ptr<std::string>, extractStringAtVA, (const addr_t, const addr_t), (override));

        MOCK_METHOD(void, stopSingleStepForVcpu, (vmi_event_t*, uint), (override));

        MOCK_METHOD(OperatingSystem, getOsType, (), (override));

        MOCK_METHOD(uint64_t, getOffset, (const std::string&), (override));

        MOCK_METHOD(addr_t, getKernelStructOffset, (const std::string&, const std::string&), (override));

        MOCK_METHOD(bool, isInitialized, (), (const override));

        MOCK_METHOD((std::tuple<addr_t, size_t, size_t>),
                    getBitfieldOffsetAndSizeFromJson,
                    (const std::string&, const std::string&),
                    (override));

        MOCK_METHOD(size_t, getStructSizeFromJson, (const std::string&), (override));

        MOCK_METHOD(void, flushV2PCache, (addr_t), (override));

        MOCK_METHOD(void, flushPageCache, (), (override));
    };
}

#endif // VMICORE_MOCK_LIBVMIINTERFACE_H
