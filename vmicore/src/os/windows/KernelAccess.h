#ifndef VMICORE_WINDOWS_KERNELACCESS_H
#define VMICORE_WINDOWS_KERNELACCESS_H

#include "../../vmi/LibvmiInterface.h"
#include "KernelOffsets.h"
#include "ProtectionValues.h"
#include <vmicore/types.h>

namespace VmiCore::Windows
{
    class IKernelAccess
    {
      public:
        virtual ~IKernelAccess() = default;

        virtual void initWindowsOffsets() = 0;

        [[nodiscard]] virtual uint64_t extractVadTreeRootAddress(uint64_t vadTreeRootNodeAddressLocation) const = 0;

        [[nodiscard]] virtual uint64_t extractImageFilePointer(uint64_t imageFilePointerAddressLocation) const = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string> extractFileName(uint64_t fileObjectBaseAddress) const = 0;

        [[nodiscard]] virtual addr_t extractControlAreaBasePointer(addr_t vadEntryBaseVA) const = 0;

        [[nodiscard]] virtual addr_t extractFilePointerObjectAddress(addr_t controlAreaBaseVA) const = 0;

        [[nodiscard]] virtual std::tuple<addr_t, addr_t>
        extractMmVadShortChildNodeAddresses(addr_t currentVadEntryBaseVA) const = 0;

        [[nodiscard]] virtual std::tuple<uint64_t, uint64_t>
        extractMmVadShortVpns(addr_t currentVadShortBaseVA) const = 0;

        [[nodiscard]] virtual addr_t getVadShortBaseVA(addr_t vadEntryBaseVA) const = 0;

        [[nodiscard]] virtual addr_t getCurrentProcessEprocessBase(addr_t currentListEntry) const = 0;

        [[nodiscard]] virtual addr_t extractDirectoryTableBase(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual pid_t extractParentID(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual std::string extractImageFileName(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual pid_t extractPID(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual uint32_t extractExitStatus(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual addr_t extractSectionAddress(addr_t eprocessBase) const = 0;

        [[nodiscard]] virtual addr_t extractControlAreaAddress(addr_t sectionAddress) const = 0;

        [[nodiscard]] virtual addr_t extractControlAreaFilePointer(addr_t controlAreaAddress) const = 0;

        [[nodiscard]] virtual std::unique_ptr<std::string> extractProcessPath(addr_t filePointerAddress) const = 0;

        [[nodiscard]] virtual addr_t getMmVadShortFlagsAddr(addr_t vadShortBaseVA) const = 0;

        [[nodiscard]] virtual uint8_t extractProtectionFlagValue(addr_t vadShortBaseVA) const = 0;

        [[nodiscard]] virtual bool extractIsPrivateMemory(addr_t vadShortBaseVA) const = 0;

        [[nodiscard]] virtual bool extractIsBeingDeleted(addr_t controlAreaBaseVA) const = 0;

        [[nodiscard]] virtual addr_t getMmSectionFlagsAddr(addr_t controlAreaBaseVA) const = 0;

        [[nodiscard]] virtual bool extractIsImage(addr_t controlAreaBaseVA) const = 0;

        [[nodiscard]] virtual bool extractIsFile(addr_t controlAreaBaseVA) const = 0;

      protected:
        IKernelAccess() = default;
    };

    class KernelAccess : public IKernelAccess
    {
      public:
        explicit KernelAccess(std::shared_ptr<ILibvmiInterface> vmiInterface);

        ~KernelAccess() override = default;

        void initWindowsOffsets() override;

        [[nodiscard]] uint64_t extractVadTreeRootAddress(uint64_t eprocessBase) const override;

        [[nodiscard]] uint64_t extractImageFilePointer(uint64_t eprocessBase) const override;

        [[nodiscard]] std::unique_ptr<std::string> extractFileName(uint64_t fileObjectBaseAddress) const override;

        [[nodiscard]] addr_t extractControlAreaBasePointer(addr_t vadEntryBaseVA) const override;

        [[nodiscard]] addr_t extractFilePointerObjectAddress(addr_t controlAreaBaseVA) const override;

        [[nodiscard]] static uint64_t removeReferenceCountFromExFastRef(uint64_t exFastRefValue);

        [[nodiscard]] std::tuple<addr_t, addr_t>
        extractMmVadShortChildNodeAddresses(addr_t currentVadEntryBaseVA) const override;

        [[nodiscard]] std::tuple<uint64_t, uint64_t> extractMmVadShortVpns(addr_t currentVadShortBaseVA) const override;

        [[nodiscard]] addr_t getVadShortBaseVA(addr_t vadEntryBaseVA) const override;

        [[nodiscard]] addr_t getCurrentProcessEprocessBase(addr_t currentListEntry) const override;

        [[nodiscard]] addr_t extractDirectoryTableBase(addr_t eprocessBase) const override;

        [[nodiscard]] pid_t extractParentID(addr_t eprocessBase) const override;

        [[nodiscard]] std::string extractImageFileName(addr_t eprocessBase) const override;

        [[nodiscard]] pid_t extractPID(addr_t eprocessBase) const override;

        [[nodiscard]] uint32_t extractExitStatus(addr_t eprocessBase) const override;

        [[nodiscard]] addr_t extractSectionAddress(addr_t eprocessBase) const override;

        [[nodiscard]] addr_t extractControlAreaAddress(addr_t sectionAddress) const override;

        [[nodiscard]] addr_t extractControlAreaFilePointer(addr_t controlAreaAddress) const override;

        [[nodiscard]] std::unique_ptr<std::string> extractProcessPath(addr_t filePointerAddress) const override;

        [[nodiscard]] addr_t getMmVadShortFlagsAddr(addr_t vadShortBaseVA) const override;

        [[nodiscard]] uint8_t extractProtectionFlagValue(addr_t vadShortBaseVA) const override;

        [[nodiscard]] bool extractIsPrivateMemory(addr_t vadShortBaseVA) const override;

        [[nodiscard]] bool extractIsBeingDeleted(addr_t controlAreaBaseVA) const override;

        [[nodiscard]] addr_t getMmSectionFlagsAddr(addr_t controlAreaBaseVA) const override;

        [[nodiscard]] bool extractIsImage(addr_t controlAreaBaseVA) const override;

        [[nodiscard]] bool extractIsFile(addr_t controlAreaBaseVA) const override;

      private:
        std::shared_ptr<ILibvmiInterface> vmiInterface;
        KernelOffsets kernelOffsets;

        [[nodiscard]] addr_t getVadNodeLeftChildOffset() const;

        [[nodiscard]] addr_t getVadNodeRightChildOffset() const;

        [[nodiscard]] uint64_t extractFlagValue(addr_t flagBaseVA, size_t size, size_t startBit, size_t endBit) const;

        template <typename T> T getFlagValue(T flags, size_t startBit, size_t endBit) const
        {
            size_t flagLength = endBit - startBit;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            T flagValueMSB = flags << (sizeof(flags) * 8 - (endBit));
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
            T flagValueLSB = flagValueMSB >> (sizeof(flags) * 8 - (flagLength));

            return flagValueLSB;
        }

        static void expectSaneKernelAddress(addr_t address, const char* caller);
    };
}

#endif // VMICORE_WINDOWS_KERNELACCESS_H
