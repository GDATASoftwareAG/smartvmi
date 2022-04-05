#ifndef OFFSET_DEFINITIONS_HPP
#define OFFSET_DEFINITIONS_HPP

#include "libvmi/libvmi.h"
#include <cstdint>

namespace OffsetDefinitionsWin10
{
    namespace _IMAGE_DATA_DIRECTORY
    {
        constexpr addr_t VirtualAddress = 0;
    }

    namespace _IMAGE_DOS_HEADER
    {
        constexpr addr_t e_lfanew = 60;
    }

    namespace _IMAGE_EXPORT_DIRECTORY
    {
        constexpr addr_t NumberOfNames = 24;
        constexpr addr_t AddressOfNameOrdinals = 36;
        constexpr addr_t AddressOfFunctions = 28;
        constexpr addr_t NumberOfFunctions = 20;
        constexpr addr_t AddressOfNames = 32;
    }

    namespace _EWOW64PROCESS
    {
        constexpr addr_t Peb = 0;
    }

    namespace _TEB
    {
        constexpr addr_t ClientId = 64;
    }

    namespace _PEB
    {
        constexpr addr_t ProcessHeaps = 240;
        constexpr addr_t Ldr = 24;
        constexpr addr_t MaximumNumberOfHeaps = 236;
        constexpr addr_t NumberOfHeaps = 232;
        constexpr addr_t ProcessHeap = 48;
    }

    namespace _OBJECT_TYPE
    {
        constexpr addr_t Index = 40;
        constexpr addr_t Name = 16;
    }

    namespace _PEB_LDR_DATA
    {
        constexpr addr_t InLoadOrderModuleList = 16;
        constexpr addr_t EntryInProgress = 64;
        constexpr addr_t InMemoryOrderModuleList = 32;
    }

    namespace _MMADDRESS_NODE
    {
    }

    namespace _FILE_OBJECT
    {
        constexpr addr_t FileName = 88;
    }

    namespace _CLIENT_ID
    {
        constexpr addr_t UniqueProcess = 0;
        constexpr addr_t UniqueThread = 8;
    }

    namespace _KPROCESS
    {
        constexpr addr_t DirectoryTableBase = 40;
    }

    namespace _ETHREAD
    {
        constexpr addr_t ThreadListEntry = 1680;
        constexpr addr_t Cid = 1576;
    }

    namespace _SECTION
    {
        constexpr addr_t u1 = 40;
    }

    namespace _EPROCESS
    {
        constexpr addr_t WoW64Process = 1064;
        constexpr addr_t ActiveProcessLinks = 752;
        constexpr addr_t UniqueProcessId = 744;
        constexpr addr_t Vm = 1280;
        constexpr addr_t VadRoot = 1552;
        constexpr addr_t ThreadListHead = 1160;
        constexpr addr_t SectionObject = 952;
        constexpr addr_t InheritedFromUniqueProcessId = 992;
        constexpr addr_t ObjectTable = 1048;
        constexpr addr_t ExitStatus = 1548;
        constexpr addr_t ImageFilePointer = 1096;
        constexpr addr_t ImageFileName = 1104;
        constexpr addr_t Peb = 1016;
    }

    namespace _NT_TIB
    {
        constexpr addr_t StackLimit = 16;
        constexpr addr_t StackBase = 8;
    }

    namespace _OBJECT_HEADER
    {
        constexpr addr_t PointerCount = 0;
        constexpr addr_t HandleCount = 8;
        constexpr addr_t Flags = 27;
        constexpr addr_t TypeIndex = 24;
        constexpr addr_t TraceFlags = 25;
        constexpr addr_t InfoMask = 26;
    }

    namespace _MMSUPPORT
    {
    }

    namespace _HANDLE_TABLE
    {
        constexpr addr_t TableCode = 8;
    }

    namespace _LIST_ENTRY
    {
        constexpr addr_t Blink = 8;
    }

    namespace _IMAGE_NT_HEADERS64
    {
        constexpr addr_t OptionalHeader = 24;
    }

    namespace _RTL_AVL_TREE
    {
        constexpr addr_t Root = 0;
    }

    namespace _CONTROL_AREA
    {
        constexpr addr_t u = 56;
        constexpr addr_t FilePointer = 64;
    }

    namespace _IMAGE_OPTIONAL_HEADER64
    {
        constexpr addr_t SizeOfImage = 56;
        constexpr addr_t NumberOfRvaAndSizes = 108;
        constexpr addr_t DataDirectory = 112;
    }

    namespace _LDR_DATA_TABLE_ENTRY
    {
        constexpr addr_t InMemoryOrderLinks = 16;
        constexpr addr_t FullDllName = 72;
        constexpr addr_t InLoadOrderLinks = 0;
        constexpr addr_t SizeOfImage = 64;
        constexpr addr_t BaseDllName = 88;
        constexpr addr_t EntryPoint = 56;
        constexpr addr_t DllBase = 48;
    }

    struct _MMSECTION_FLAGS
    {
        uint32_t BeingDeleted : 1;
        uint32_t BeingCreated : 1;
        uint32_t BeingPurged : 1;
        uint32_t NoModifiedWriting : 1;
        uint32_t FailAllIo : 1;
        uint32_t Image : 1;
        uint32_t Based : 1;
        uint32_t File : 1;
        uint32_t AttemptingDelete : 1;
        uint32_t PrefetchCreated : 1;
        uint32_t PhysicalMemory : 1;
        uint32_t CopyOnWrite : 1;
        uint32_t Reserve : 1;
        uint32_t Commit : 1;
        uint32_t NoChange : 1;
        uint32_t WasPurged : 1;
        uint32_t UserReference : 1;
        uint32_t GlobalMemory : 1;
        uint32_t DeleteOnClose : 1;
        uint32_t FilePointerNull : 1;
        uint32_t PreferredNode : 6;
        uint32_t GlobalOnlyPerSession : 1;
        uint32_t UserWritable : 1;
        uint32_t SystemVaAllocated : 1;
        uint32_t PreferredFsCompressionBoundary : 1;
        uint32_t UsingFileExtents : 1;
        uint32_t Spare : 1;
    };

    struct _MMVAD_FLAGS
    {
        uint32_t VadType : 3;
        uint32_t Protection : 5;
        uint32_t PreferredNode : 6;
        uint32_t NoChange : 1;
        uint32_t PrivateMemory : 1;
        uint32_t PrivateFixup : 1;
        uint32_t ManySubsections : 1;
        uint32_t Enclave : 1;
        uint32_t DeleteInProgress : 1;
        uint32_t Spare : 12;
    };
}

namespace windowsWOW64
{
    namespace _PEB
    {
        constexpr addr_t ProcessHeaps = 144;
        constexpr addr_t Ldr = 12;
        constexpr addr_t MaximumNumberOfHeaps = 140;
        constexpr addr_t NumberOfHeaps = 136;
        constexpr addr_t ProcessHeap = 24;
    }

    namespace _IMAGE_DATA_DIRECTORY
    {
        constexpr addr_t VirtualAddress = 0;
    }

    namespace _LIST_ENTRY
    {
        constexpr addr_t Blink = 4;
    }

    namespace _OBJECT_TYPE
    {
        constexpr addr_t Index = 76;
        constexpr addr_t Name = 64;
    }

    namespace _IMAGE_DOS_HEADER
    {
        constexpr addr_t e_lfanew = 60;
    }

    namespace _PEB_LDR_DATA
    {
        constexpr addr_t InLoadOrderModuleList = 12;
        constexpr addr_t EntryInProgress = 36;
        constexpr addr_t InMemoryOrderModuleList = 20;
    }

    namespace _EPROCESS
    {
        constexpr addr_t ActiveProcessLinks = 136;
        constexpr addr_t UniqueProcessId = 132;
        constexpr addr_t Vm = 504;
        constexpr addr_t VadRoot = 284;
        constexpr addr_t InheritedFromUniqueProcessId = 332;
        constexpr addr_t ObjectTable = 196;
        constexpr addr_t ImageFileName = 372;
        constexpr addr_t Peb = 432;
    }

    namespace _IMAGE_EXPORT_DIRECTORY
    {
        constexpr addr_t NumberOfNames = 24;
        constexpr addr_t AddressOfNameOrdinals = 36;
        constexpr addr_t AddressOfFunctions = 28;
        constexpr addr_t NumberOfFunctions = 20;
        constexpr addr_t AddressOfNames = 32;
    }

    namespace _NT_TIB
    {
        constexpr addr_t StackLimit = 8;
        constexpr addr_t StackBase = 4;
    }

    namespace _OBJECT_HEADER
    {
        constexpr addr_t PointerCount = 0;
        constexpr addr_t NameInfoOffset = 12;
        constexpr addr_t HandleInfoOffset = 13;
        constexpr addr_t HandleCount = 4;
        constexpr addr_t Flags = 15;
        constexpr addr_t QuotaInfoOffset = 14;
        constexpr addr_t Type = 8;
    }

    namespace _IMAGE_NT_HEADERS
    {
        constexpr addr_t OptionalHeader = 24;
    }

    namespace _LDR_DATA_TABLE_ENTRY
    {
        constexpr addr_t InMemoryOrderLinks = 8;
        constexpr addr_t FullDllName = 36;
        constexpr addr_t InLoadOrderLinks = 0;
        constexpr addr_t SizeOfImage = 32;
        constexpr addr_t BaseDllName = 44;
        constexpr addr_t EntryPoint = 28;
        constexpr addr_t DllBase = 24;
    }

    namespace _MMVAD
    {
        constexpr addr_t LeftChild = 12;
        constexpr addr_t RightChild = 16;
        constexpr addr_t EndingVpn = 4;
        constexpr addr_t StartingVpn = 0;
    }

    namespace _KPROCESS
    {
        constexpr addr_t DirectoryTableBase = 24;
    }

    namespace _HANDLE_TABLE
    {
        constexpr addr_t TableCode = 0;
        constexpr addr_t HandleCount = 60;
    }
}

#endif // OFFSET_DEFINITIONS_HPP