#include "PathExtractor.h"
#include "Constants.h"

namespace Linux
{
    PathExtractor::PathExtractor(std::shared_ptr<ILibvmiInterface> vmiInterface,
                                 const std::shared_ptr<ILogging>& logging)
        : vmiInterface(std::move(vmiInterface)), logger(NEW_LOGGER(logging))
    {
    }

    std::string PathExtractor::extractDPath(uint64_t path) const
    {
        if (path == 0)
        {
            return {};
        }

        const auto mnt = vmiInterface->read64VA(path + vmiInterface->getKernelStructOffset("path", "mnt"),
                                                vmiInterface->convertPidToDtb(SYSTEM_PID));
        const auto dentry = vmiInterface->read64VA(path + vmiInterface->getKernelStructOffset("path", "dentry"),
                                                   vmiInterface->convertPidToDtb(SYSTEM_PID));

        if (dentry == 0 || mnt == 0)
        {
            return {};
        }

        return createPath(dentry, mnt - vmiInterface->getKernelStructOffset("mount", "mnt"));
    }

    std::string PathExtractor::createPath(uint64_t dentry, uint64_t mnt) const
    {
        std::string path;
        try
        {
            const auto name = vmiInterface->extractStringAtVA(
                vmiInterface->read64VA(dentry + vmiInterface->getKernelStructOffset("dentry", "d_name") +
                                           vmiInterface->getKernelStructOffset("qstr", "name"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID)),
                vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto parent =
                vmiInterface->read64VA(dentry + vmiInterface->getKernelStructOffset("dentry", "d_parent"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto mntRoot = vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt"),
                                                        vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto mntMountpoint =
                vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt_mountpoint"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));
            const auto mntParent =
                vmiInterface->read64VA(mnt + vmiInterface->getKernelStructOffset("mount", "mnt_parent"),
                                       vmiInterface->convertPidToDtb(SYSTEM_PID));

            if (parent != dentry && dentry != mntRoot)
            {
                path.append(createPath(parent, mnt));
            }
            else if (mntParent != mnt)
            {
                path.append(createPath(mntMountpoint, mntParent));
            }

            if ((parent == dentry && name->at(0) == '/') || dentry == mntRoot)
            {
                return path;
            }
            if (parent == dentry)
            {
                return path.append(*name);
            }
            return path.append(fmt::format("/{}", *name));
        }
        catch (const std::exception& e)
        {
            logger->warning("Unable to extract part of a path.", {logfield::create("exception", e.what())});
        }

        return path;
    }
}
