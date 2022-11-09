#ifndef VMICORE_VMIINITDATA_H
#define VMICORE_VMIINITDATA_H

#include <filesystem>
#include <gsl/pointers>
#include <libvmi/libvmi.h>

namespace VmiCore
{
    class VmiInitData
    {
      public:
        gsl::owner<vmi_init_data_t*> data{};

        explicit VmiInitData(const std::filesystem::path& socketPath);

        // do not allow copy construction therefore avoiding multiple free() calls to the same memory location during
        // object destruction
        VmiInitData(const VmiInitData&) = delete;

        VmiInitData(VmiInitData&& vmiInitData) noexcept;

        ~VmiInitData();
    };
}

#endif // VMICORE_VMIINITDATA_H
