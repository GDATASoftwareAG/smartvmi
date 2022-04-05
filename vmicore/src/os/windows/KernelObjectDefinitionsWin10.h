#ifndef VMICORE_WIN10OBJECTDEFINITIONS_H
#define VMICORE_WIN10OBJECTDEFINITIONS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#include <cstdint>

namespace KernelObjectDefinitionsWin10
{
    typedef struct _UNICODE_STRING
    {
        uint16_t Length;
        uint16_t MaximumLength;
        uint32_t padding;
        uint16_t* Buffer;

        bool operator==(const _UNICODE_STRING& rhs) const
        {
            return (Length != rhs.Length)                 ? false
                   : (MaximumLength != rhs.MaximumLength) ? false
                   : (padding != rhs.padding)             ? false
                   : (Buffer != rhs.Buffer)               ? false
                                                          : true;
        }

        bool operator!=(const _UNICODE_STRING& rhs) const
        {
            return !operator==(rhs);
        }
    } _UNICODE_STRING, *_UNICODE_STRING_t;

    typedef struct _FILE_OBJECT
    {
        int16_t Type;
        int16_t Size;
        uint32_t padding;
        struct _DEVICE_OBJECT* DeviceObject;
        struct _VPB* Vpb;
        void* FsContext;
        void* FsContext2;
        struct _SECTION_OBJECT_POINTERS* SectionObjectPointer;
        void* PrivateCacheMap;
        int32_t FinalStatus;
        uint32_t padding2;
        struct _FILE_OBJECT* RelatedFileObject;
        uint8_t LockOperation;
        uint8_t DeletePending;
        uint8_t ReadAccess;
        uint8_t WriteAccess;
        uint8_t DeleteAccess;
        uint8_t SharedRead;
        uint8_t SharedWrite;
        uint8_t SharedDelete;
        uint32_t Flags;
        uint32_t padding3;
        struct _UNICODE_STRING FileName;

        bool operator==(const _FILE_OBJECT& rhs) const
        {
            return (Type != rhs.Type)                                   ? false
                   : (Size != rhs.Size)                                 ? false
                   : (DeviceObject != rhs.DeviceObject)                 ? false
                   : (Vpb != rhs.Vpb)                                   ? false
                   : (FsContext != rhs.FsContext)                       ? false
                   : (FsContext2 != rhs.FsContext2)                     ? false
                   : (SectionObjectPointer != rhs.SectionObjectPointer) ? false
                   : (PrivateCacheMap != rhs.PrivateCacheMap)           ? false
                   : (FinalStatus != rhs.FinalStatus)                   ? false
                   : (RelatedFileObject != rhs.RelatedFileObject)       ? false
                   : (LockOperation != rhs.LockOperation)               ? false
                   : (DeletePending != rhs.DeletePending)               ? false
                   : (ReadAccess != rhs.ReadAccess)                     ? false
                   : (WriteAccess != rhs.WriteAccess)                   ? false
                   : (DeleteAccess != rhs.DeleteAccess)                 ? false
                   : (SharedRead != rhs.SharedRead)                     ? false
                   : (SharedWrite != rhs.SharedWrite)                   ? false
                   : (SharedDelete != rhs.SharedDelete)                 ? false
                   : (Flags != rhs.Flags)                               ? false
                   : (padding3 != rhs.padding3)                         ? false
                   : (FileName != rhs.FileName)                         ? false
                                                                        : true;
        }

        bool operator!=(const _FILE_OBJECT& rhs) const
        {
            return !operator==(rhs);
        }
    } _FILE_OBJECT, *_FILE_OBJECT_t;

    typedef struct _RTL_BALANCED_NODE
    {
        union
        {
            struct _RTL_BALANCED_NODE* Children[2];
            struct
            {
                struct _RTL_BALANCED_NODE* Left;
                struct _RTL_BALANCED_NODE* Right;
            };
        } balancedNodeChildren;
        union
        {
            union
            {
                uint8_t Red : 1;
                uint8_t Balance : 2;
            };
            uint64_t ParentValue;
        };

        bool operator==(const _RTL_BALANCED_NODE& rhs) const
        {
            return (balancedNodeChildren.Children[0] != rhs.balancedNodeChildren.Children[0])   ? false
                   : (balancedNodeChildren.Children[1] != rhs.balancedNodeChildren.Children[1]) ? false
                   : (balancedNodeChildren.Left != rhs.balancedNodeChildren.Left)               ? false
                   : (balancedNodeChildren.Right != rhs.balancedNodeChildren.Right)             ? false
                   : (Red != rhs.Red)                                                           ? false
                   : (Balance != rhs.Balance)                                                   ? false
                   : (ParentValue != rhs.ParentValue)                                           ? false
                                                                                                : true;
        }

        bool operator!=(const _RTL_BALANCED_NODE& rhs) const
        {
            return !operator==(rhs);
        }
    } _RTL_BALANCED_NODE, *_RTL_BALANCED_NODE_t;

    typedef struct _EX_PUSH_LOCK
    {
        union
        {
            void* Ptr;
            uint64_t Value;
            struct
            {
                uint64_t Locked : 1;
                uint64_t Waiting : 1;
                uint64_t Waking : 1;
                uint64_t MultipleShared : 1;
                uint64_t Shared : 60;
            };
        };

        bool operator==(const _EX_PUSH_LOCK& rhs) const
        {
            return (Ptr != rhs.Ptr)                         ? false
                   : (Value != rhs.Value)                   ? false
                   : (Locked != rhs.Locked)                 ? false
                   : (Waiting != rhs.Waiting)               ? false
                   : (Waking != rhs.Waking)                 ? false
                   : (MultipleShared != rhs.MultipleShared) ? false
                   : (Shared != rhs.Shared)                 ? false
                                                            : true;
        }

        bool operator!=(const _EX_PUSH_LOCK& rhs) const
        {
            return !operator==(rhs);
        }
    } _EX_PUSH_LOCK, *_EX_PUSH_LOCK_t;

    typedef struct _MMVAD_FLAGS
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

        bool operator==(const _MMVAD_FLAGS& rhs) const
        {
            return (VadType != rhs.VadType)                     ? false
                   : (Protection != rhs.Protection)             ? false
                   : (PreferredNode != rhs.PreferredNode)       ? false
                   : (NoChange != rhs.NoChange)                 ? false
                   : (PrivateMemory != rhs.PrivateMemory)       ? false
                   : (PrivateFixup != rhs.PrivateFixup)         ? false
                   : (ManySubsections != rhs.ManySubsections)   ? false
                   : (Enclave != rhs.Enclave)                   ? false
                   : (DeleteInProgress != rhs.DeleteInProgress) ? false
                   : (Spare != rhs.Spare)                       ? false
                                                                : true;
        }

        bool operator!=(const _MMVAD_FLAGS& rhs) const
        {
            return !operator==(rhs);
        }
    } MMVAD_FLAGS, *MMVAD_FLAGS_t;

    typedef struct _MMVAD_FLAGS1
    {
        uint32_t CommitCharge : 31;
        uint32_t MemCommit : 1;

        bool operator==(const _MMVAD_FLAGS1& rhs) const
        {
            return (CommitCharge != rhs.CommitCharge) ? false : (MemCommit != rhs.MemCommit) ? false : true;
        }

        bool operator!=(const _MMVAD_FLAGS1& rhs) const
        {
            return !operator==(rhs);
        }
    } MMVAD_FLAGS1, *MMVAD_FLAGS1_t;

    typedef struct _MMVAD_SHORT
    {
        union
        {
            struct _MMVAD_SHORT* NextVad;
            struct _RTL_BALANCED_NODE VadNode;
        };
        uint32_t StartingVpn;
        uint32_t EndingVpn;
        uint8_t StartingVpnHigh;
        uint8_t EndingVpnHigh;
        uint8_t CommitChargeHigh;
        uint8_t SpareNT64VadUChar;
        int32_t ReferenceCount;
        struct _EX_PUSH_LOCK PushLock;
        union
        {
            uint32_t LongFlags;
            struct _MMVAD_FLAGS VadFlags;
        } u;
        union
        {
            uint32_t LongFlags1;
            struct _MMVAD_FLAGS1 VadFlags1;
        } u1;
        struct _MI_VAD_EVENT_BLOCK* EventList;

        bool operator==(const _MMVAD_SHORT& rhs) const
        {
            return (NextVad != rhs.NextVad)                       ? false
                   : (VadNode != rhs.VadNode)                     ? false
                   : (StartingVpn != rhs.StartingVpn)             ? false
                   : (EndingVpn != rhs.EndingVpn)                 ? false
                   : (StartingVpnHigh != rhs.StartingVpnHigh)     ? false
                   : (EndingVpnHigh != rhs.EndingVpnHigh)         ? false
                   : (CommitChargeHigh != rhs.CommitChargeHigh)   ? false
                   : (SpareNT64VadUChar != rhs.SpareNT64VadUChar) ? false
                   : (ReferenceCount != rhs.ReferenceCount)       ? false
                   : (PushLock != rhs.PushLock)                   ? false
                   : (u.LongFlags != rhs.u.LongFlags)             ? false
                   : (u.VadFlags != rhs.u.VadFlags)               ? false
                   : (u1.LongFlags1 != rhs.u1.LongFlags1)         ? false
                   : (u1.VadFlags1 != rhs.u1.VadFlags1)           ? false
                   : (EventList != rhs.EventList)                 ? false
                                                                  : true;
        }

        bool operator!=(const _MMVAD_SHORT& rhs) const
        {
            return !operator==(rhs);
        }
    } _MMVAD_SHORT, *_MMVAD_SHORT_t;

    typedef struct _LIST_ENTRY
    {
        struct _LIST_ENTRY* Flink;
        struct _LIST_ENTRY* Blink;

        bool operator==(const _LIST_ENTRY& rhs) const
        {
            return (Flink != rhs.Flink) ? false : (Blink != rhs.Blink) ? false : true;
        }

        bool operator!=(const _LIST_ENTRY& rhs) const
        {
            return !operator==(rhs);
        }
    } _LIST_ENTRY, *_LIST_ENTRY_t;

    typedef struct _MMSECTION_FLAGS
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
    } _MMSECTION_FLAGS, *_MMSECTION_FLAGS_t;

    typedef struct _MMSECTION_FLAGS2
    {
        uint32_t PartitionId : 10;
        uint32_t NumberOfChildViews : 22;
    } _MMSECTION_FLAGS2, *_MMSECTION_FLAGS2_t;

    typedef struct _EX_FAST_REF
    {
        void* Object;

        bool operator==(const _EX_FAST_REF& rhs) const
        {
            return (Object != rhs.Object) ? false : true;
        }

        bool operator!=(const _EX_FAST_REF& rhs) const
        {
            return !operator==(rhs);
        }
    } _EX_FAST_REF, *_EX_FAST_REF_t;

    typedef struct _CONTROL_AREA
    {
        struct _SEGMENT* Segment;
        struct _LIST_ENTRY ListHead;
        uint64_t NumberOfSectionReferences;
        uint64_t NumberOfPfnReferences;
        uint64_t NumberOfMappedViews;
        uint64_t NumberOfUserReferences;
        union
        {
            struct _MMSECTION_FLAGS Flags;
            uint32_t LongFlags;
        } u;
        union
        {
            struct _MMSECTION_FLAGS2 Flags;
            uint32_t LongFlags;
        } u1;
        struct _EX_FAST_REF FilePointer;
        int32_t ControlAreaLock;
        uint32_t ModifiedWriteCount;
        struct _MI_CONTROL_AREA_WAIT_BLOCK* WaitList;
        uint64_t u2; // union{	}u2;
        uint64_t u3; // union{	}u2;
        uint64_t LockedPages;
        struct _EX_PUSH_LOCK* FileObjectLock;

    } _CONTROL_AREA, *_CONTROL_AREA_t;

    typedef struct _MMSUBSECTION_FLAGS
    {
        uint32_t SubsectionAccessed : 1;
        uint32_t GlobalMemory : 1;
        uint32_t SubsectionMappedDirect : 1;
        uint32_t OnDereferenceList : 1;
        uint32_t Protection : 2;
        uint32_t SectorEndOffset : 10;
    } _MMSUBSECTION_FLAGS, *_MMSUBSECTION_FLAGS_t;

    typedef struct _SUBSECTION
    {
        struct _CONTROL_AREA* ControlArea;
        struct _MMPTE* SubsectionBase;
        struct _SUBSECTION* NextSubsection;
        union
        {
            struct _MI_SUBSECTION_WAIT_BLOCK* CreationWaitList;
            struct _MI_PER_SESSION_PROTOS* SessionDriverProtos;
            struct _RTL_AVL_TREE* GlobalPerSessionHead;
        };
        union
        {
            uint32_t LongFlags1;
            struct _MMSUBSECTION_FLAGS SubsectionFlags;
        } u;
        uint32_t StartingSector;
        uint32_t NumberOfFullSectors;
        uint32_t PtesInSubsection;
        union
        {
            uint32_t NumberOfChildViews;
        } u1;
        union
        {
            struct
            {
                uint32_t UnusedPtes : 31;
                uint32_t DirtyPages : 1;
            } u1;
            struct
            {
                uint32_t AlignmentNoAccessPtes : 31;
                uint32_t DirtyPages : 1;
            } u2;
        } u2;

        bool operator==(const _SUBSECTION& rhs) const
        {
            return (ControlArea != rhs.ControlArea)                   ? false
                   : (SubsectionBase != rhs.SubsectionBase)           ? false
                   : (NextSubsection != rhs.NextSubsection)           ? false
                   : (StartingSector != rhs.StartingSector)           ? false
                   : (NumberOfFullSectors != rhs.NumberOfFullSectors) ? false
                   : (PtesInSubsection != rhs.PtesInSubsection)       ? false
                                                                      : true;
        }

        bool operator!=(const _SUBSECTION& rhs) const
        {
            return !operator==(rhs);
        }
    } _SUBSECTION, *_SUBSECTION_t;

    typedef struct _MMVAD
    {
        struct _MMVAD_SHORT Core;
        union
        {
            uint32_t LongFlags2;
            struct _MMVAD_FLAGS2;
        } u2;
        struct _SUBSECTION* Subsection;
        void* FirstPrototypePte; // struct _MMPTE* FirstPrototypePte;
        void* LastContiguousPte; // struct _MMPTE* LastContiguousPte;
        struct _LIST_ENTRY ViewLinks;
        struct _EPROCESS* VadsProcess;
        union
        {
            void* ExtendedInfo;    // struct _MMEXTEND_INFO* ExtendedInfo;
            uint64_t SequentialVa; // struct _MI_VAD_SEQUENTIAL_INFO SequentialVa;
        } u4;
        struct _FILE_OBJECT* FileObject;

        bool operator==(const _MMVAD& rhs) const
        {
            return (Core != rhs.Core)                             ? false
                   : (u2.LongFlags2 != rhs.u2.LongFlags2)         ? false
                   : (Subsection != rhs.Subsection)               ? false
                   : (FirstPrototypePte != rhs.FirstPrototypePte) ? false
                   : (LastContiguousPte != rhs.LastContiguousPte) ? false
                   : (ViewLinks != rhs.ViewLinks)                 ? false
                   : (VadsProcess != rhs.VadsProcess)             ? false
                   : (u4.ExtendedInfo != rhs.u4.ExtendedInfo)     ? false
                   : (u4.SequentialVa != rhs.u4.SequentialVa)     ? false
                   : (FileObject != rhs.FileObject)               ? false
                                                                  : true;
        }

        bool operator!=(const _MMVAD& rhs) const
        {
            return !operator==(rhs);
        }

    } _MMVAD, *_MMVAD_t;

    enum class ProtectionValues
    {
        PAGE_NOACCESS,
        PAGE_READONLY,
        PAGE_EXECUTE,
        PAGE_EXECUTE_READ,
        PAGE_READWRITE,
        PAGE_WRITECOPY,
        PAGE_EXECUTE_READWRITE,
        PAGE_EXECUTE_WRITECOPY,
        PAGE_NOACCESS_2,
        PAGE_NOCACHE_PAGE_READONLY,
        PAGE_NOCACHE_PAGE_EXECUTE,
        PAGE_NOCACHE_PAGE_EXECUTE_READ,
        PAGE_NOCACHE_PAGE_READWRITE,
        PAGE_NOCACHE_PAGE_WRITECOPY,
        PAGE_NOCACHE_PAGE_EXECUTE_READWRITE,
        PAGE_NOCACHE_PAGE_EXECUTE_WRITECOPY,
        PAGE_NOACCESS_3,
        PAGE_GUARD_PAGE_READONLY,
        PAGE_GUARD_PAGE_EXECUTE,
        PAGE_GUARD_PAGE_EXECUTE_READ,
        PAGE_GUARD_PAGE_READWRITE,
        PAGE_GUARD_PAGE_WRITECOPY,
        PAGE_GUARD_PAGE_EXECUTE_READWRITE,
        PAGE_GUARD_PAGE_EXECUTE_WRITECOPY,
        PAGE_NOACCESS_4,
        PAGE_WRITECOMBINE_PAGE_READONLY,
        PAGE_WRITECOMBINE_PAGE_EXECUTE,
        PAGE_WRITECOMBINE_PAGE_EXECUTE_READ,
        PAGE_WRITECOMBINE_PAGE_READWRITE,
        PAGE_WRITECOMBINE_PAGE_WRITECOPY,
        PAGE_WRITECOMBINE_PAGE_EXECUTE_READWRITE,
        PAGE_WRITECOMBINE_PAGE_EXECUTE_WRITECOPY,
    };
}

#pragma GCC diagnostic pop

#endif // VMICORE_WIN10OBJECTDEFINITIONS_H
