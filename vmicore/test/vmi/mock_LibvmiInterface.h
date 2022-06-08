#ifndef VMICORE_MOCK_LIBVMIINTERFACE_H
#define VMICORE_MOCK_LIBVMIINTERFACE_H

#include "../../src/vmi/LibvmiInterface.h"
#include <gmock/gmock.h>

class MockLibvmiInterface : public ILibvmiInterface
{
  public:
    MOCK_METHOD(void, initializeVmi, (const std::function<void()>& postInitializationFunction), (override));

    MOCK_METHOD(void, waitForCR3Event, (const std::function<void()>& cr3EventHandler), (override));

    MOCK_METHOD(void, clearEvent, (vmi_event_t & event, bool deallocate), (override));

    MOCK_METHOD(uint8_t, read8PA, (const uint64_t pyhsicalAddress), (override));

    MOCK_METHOD(uint8_t, read8VA, (const uint64_t virtualAddress, const uint64_t cr3), (override));

    MOCK_METHOD(uint32_t, read32VA, (const uint64_t virtualAddress, const uint64_t cr3), (override));

    MOCK_METHOD(uint64_t, read64VA, (const uint64_t virtualAddress, const uint64_t cr3), (override));

    MOCK_METHOD(bool,
                readXVA,
                (const uint64_t virtualAddress, const uint64_t cr3, std::vector<uint8_t>& content),
                (override));

    MOCK_METHOD(void, write8PA, (const uint64_t physicalAddress, const uint8_t value), (override));

    MOCK_METHOD(void, waitForEvent, (), (override));

    MOCK_METHOD(void, registerEvent, (vmi_event_t & event), (override));

    MOCK_METHOD(uint64_t, getCurrentVmId, (), (override));

    MOCK_METHOD(uint, getNumberOfVCPUs, (), (override));

    MOCK_METHOD(uint64_t, translateKernelSymbolToVA, (const std::string& kernelSymbolName), (override));

    MOCK_METHOD(uint64_t, convertVAToPA, (uint64_t virtualAddress, uint64_t cr3Register), (override));

    MOCK_METHOD(uint64_t, convertPidToDtb, (pid_t processID), (override));

    MOCK_METHOD(pid_t, convertDtbToPid, (uint64_t dtb), (override));

    MOCK_METHOD(void, pauseVm, (), (override));

    MOCK_METHOD(void, resumeVm, (), (override));

    MOCK_METHOD(bool, isVmAlive, (), (override));

    MOCK_METHOD(bool, areEventsPending, (), (override));

    MOCK_METHOD(std::unique_ptr<std::string>,
                extractUnicodeStringAtVA,
                (const uint64_t stringVA, const uint64_t cr3),
                (override));

    MOCK_METHOD(std::unique_ptr<std::string>,
                extractStringAtVA,
                (const uint64_t virtualAddress, const uint64_t cr3),
                (override));

    MOCK_METHOD(void, stopSingleStepForVcpu, (vmi_event_t * event, uint vcpuId), (override));

    MOCK_METHOD(uint64_t, getSystemCr3, (), (override));

    MOCK_METHOD(addr_t, getKernelStructOffset, (const std::string& structName, const std::string& member), (override));

    MOCK_METHOD(bool, isInitialized, (), (override));

    MOCK_METHOD((std::tuple<addr_t, size_t, size_t>),
                getBitfieldOffsetAndSizeFromJson,
                (const std::string&, const std::string&),
                (override));

    MOCK_METHOD(size_t, getStructSizeFromJson, (const std::string& struct_name), (override));
};

#endif // VMICORE_MOCK_LIBVMIINTERFACE_H
