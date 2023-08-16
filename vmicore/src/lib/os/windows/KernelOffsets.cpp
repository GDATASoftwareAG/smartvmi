#include "KernelOffsets.h"
#include "Constants.h"
#include <fmt/core.h>
#include <source_location>

namespace VmiCore::Windows
{
    KernelOffsets KernelOffsets::init(const std::shared_ptr<ILibvmiInterface>& vmiInterface)
    {
        if (!vmiInterface->isInitialized())
        {
            throw std::invalid_argument(fmt::format("{}: Aborting, vmiInterface not initialized yet.",
                                                    std::source_location::current().function_name()));
        }

        KernelOffsets kernelOffsets{
            .mmvadFlags = {.protection{vmiInterface->getBitfieldOffsetAndSizeFromJson(
                               KernelStructOffsets::mmvad_flags::structName, "Protection")},
                           .privateMemory{vmiInterface->getBitfieldOffsetAndSizeFromJson(
                               KernelStructOffsets::mmvad_flags::structName, "PrivateMemory")}},
            .mmsectionFlags = {.beingDeleted{vmiInterface->getBitfieldOffsetAndSizeFromJson(
                                   KernelStructOffsets::mmsection_flags::structName, "BeingDeleted")},
                               .image{vmiInterface->getBitfieldOffsetAndSizeFromJson(
                                   KernelStructOffsets::mmsectionFlags::structName, "Image")},
                               .file{vmiInterface->getBitfieldOffsetAndSizeFromJson(
                                   KernelStructOffsets::mmsectionFlags::structName, "File")}},
            .mmVadShort = {.VadNode = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "VadNode"),
                           .StartingVpn = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "StartingVpn"),
                           .StartingVpnHigh = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "StartingVpnHigh"),
                           .EndingVpn = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "EndingVpn"),
                           .EndingVpnHigh = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "EndingVpnHigh"),
                           .Flags = vmiInterface->getKernelStructOffset("_MMVAD_SHORT", "u")},
            .eprocess = {.ActiveProcessLinks = vmiInterface->getKernelStructOffset("_EPROCESS", "ActiveProcessLinks"),
                         .UniqueProcessId = vmiInterface->getKernelStructOffset("_EPROCESS", "UniqueProcessId"),
                         .VadRoot = vmiInterface->getKernelStructOffset("_EPROCESS", "VadRoot"),
                         .SectionObject = vmiInterface->getKernelStructOffset("_EPROCESS", "SectionObject"),
                         .InheritedFromUniqueProcessId =
                             vmiInterface->getKernelStructOffset("_EPROCESS", "InheritedFromUniqueProcessId"),
                         .ExitStatus = vmiInterface->getKernelStructOffset("_EPROCESS", "ExitStatus"),
                         .ImageFilePointer = vmiInterface->getKernelStructOffset("_EPROCESS", "ImageFilePointer"),
                         .ImageFileName = vmiInterface->getKernelStructOffset("_EPROCESS", "ImageFileName"),
                         .WoW64Process = vmiInterface->getKernelStructOffset("_EPROCESS", "WoW64Process")},
            .controlArea = {._mmsection_flags = vmiInterface->getKernelStructOffset("_CONTROL_AREA", "u"),
                            .FilePointer = vmiInterface->getKernelStructOffset("_CONTROL_AREA", "FilePointer")},
            .mmVad = {.mmVadShortBaseAddress = vmiInterface->getKernelStructOffset("_MMVAD", "Core"),
                      .Subsection = vmiInterface->getKernelStructOffset("_MMVAD", "Subsection")},
            .rtlBalancedNode = {.Left = vmiInterface->getKernelStructOffset("_RTL_BALANCED_NODE", "Left"),
                                .Right = vmiInterface->getKernelStructOffset("_RTL_BALANCED_NODE", "Right")},
            .fileObject = {.FileName = vmiInterface->getKernelStructOffset("_FILE_OBJECT", "FileName")},
            .section = {.controlArea = vmiInterface->getKernelStructOffset("_SECTION", "u1")},
            .kprocess = {.directoryTableBase = vmiInterface->getKernelStructOffset("_KPROCESS", "DirectoryTableBase"),
                         .userDirectoryTableBase =
                             vmiInterface->getWindowsBuild() >= winBuildRedstone4
                                 ? vmiInterface->getKernelStructOffset("_KPROCESS", "UserDirectoryTableBase")
                                 : vmiInterface->getKernelStructOffset("_KPROCESS", "DirectoryTableBase")},
            .subSection = {.ControlArea = vmiInterface->getKernelStructOffset("_SUBSECTION", "ControlArea")},
            .exFastRef = {.Object = vmiInterface->getKernelStructOffset("_EX_FAST_REF", "Object")}};

        return kernelOffsets;
    }
}
