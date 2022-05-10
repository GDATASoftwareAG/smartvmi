#ifndef VMICORE_VMIINITDATA_H
#define VMICORE_VMIINITDATA_H

#include <filesystem>
#include <gsl/pointers>
#include <libvmi/libvmi.h>

class VmiInitData
{
  public:
    gsl::owner<vmi_init_data_t*> data{};

    explicit VmiInitData(const std::filesystem::path& socketPath);

    ~VmiInitData();
};

#endif // VMICORE_VMIINITDATA_H
