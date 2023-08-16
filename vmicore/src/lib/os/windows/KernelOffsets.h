#ifndef VMICORE_WINDOWS_KERNELOFFSETS_H
#define VMICORE_WINDOWS_KERNELOFFSETS_H

#include "../../vmi/LibvmiInterface.h"
#include <memory>

namespace VmiCore::Windows
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    namespace KernelStructOffsets
    {
        using _control_area = struct _control_area
        {
            addr_t _mmsection_flags;
            addr_t FilePointer;
        } __attribute__((aligned(16)));

        using _file_object = struct _file_object
        {
            addr_t FileName;
        };

        using _section = struct _section
        {
            addr_t controlArea;
        };

        using _kprocess = struct _kprocess
        {
            addr_t directoryTableBase;
            addr_t userDirectoryTableBase;
        };

        using _eprocess = struct _eprocess
        {
            addr_t ActiveProcessLinks;
            addr_t UniqueProcessId;
            addr_t VadRoot;
            addr_t SectionObject;
            addr_t InheritedFromUniqueProcessId;
            addr_t ExitStatus;
            addr_t ImageFilePointer;
            addr_t ImageFileName;
            addr_t WoW64Process;
        } __attribute__((aligned(64)));

        using _mmvad_short = struct _mmvad_short
        {
            addr_t VadNode;
            addr_t StartingVpn;
            addr_t StartingVpnHigh;
            addr_t EndingVpn;
            addr_t EndingVpnHigh;
            addr_t Flags;
        } __attribute__((aligned(64)));

        using _mmvad = struct _mmvad
        {
            addr_t mmVadShortBaseAddress;
            addr_t Subsection;
        } __attribute__((aligned(16)));

        using _subsection = struct _subsection
        {
            addr_t ControlArea;
        };

        using _ex_fast_ref = struct _ex_fast_ref
        {
            addr_t Object;
        };

        using _rtl_balanced_node = struct _rtl_balanced_node
        {
            addr_t Left;
            addr_t Right;
        } __attribute__((aligned(16)));

        using _flag = struct _flag
        {
            _flag() = default;

            _flag(const std::tuple<addr_t, size_t, size_t>& flagInfo) // NOLINT(google-explicit-constructor)
                : offset(std::get<0>(flagInfo)), startBit(std::get<1>(flagInfo)), endBit(std::get<2>(flagInfo)){};

            addr_t offset;
            size_t startBit;
            size_t endBit;
        } __attribute__((aligned(32)));

        using mmvadFlags = struct mmvad_flags
        {
            constexpr static const char* structName = "_MMVAD_FLAGS";
            _flag protection;
            _flag privateMemory;
        } __attribute__((aligned(128)));

        using mmsectionFlags = struct mmsection_flags
        {
            constexpr static const char* structName = "_MMSECTION_FLAGS";
            _flag beingDeleted;
            _flag image;
            _flag file;
        } __attribute__((aligned(128)));
    } // namespace KernelStructOffsets
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

    class KernelOffsets
    {
      public:
        static KernelOffsets init(const std::shared_ptr<ILibvmiInterface>& vmiInterface);

        KernelStructOffsets::mmvad_flags mmvadFlags{};
        KernelStructOffsets::mmsection_flags mmsectionFlags{};
        KernelStructOffsets::_mmvad_short mmVadShort{};
        KernelStructOffsets::_eprocess eprocess{};
        KernelStructOffsets::_control_area controlArea{};
        KernelStructOffsets::_mmvad mmVad{};
        KernelStructOffsets::_rtl_balanced_node rtlBalancedNode{};
        KernelStructOffsets::_file_object fileObject{};
        KernelStructOffsets::_section section{};
        KernelStructOffsets::_kprocess kprocess{};
        KernelStructOffsets::_subsection subSection{};
        KernelStructOffsets::_ex_fast_ref exFastRef{};
    };
}

#endif // VMICORE_WINDOWS_KERNELOFFSETS_H
