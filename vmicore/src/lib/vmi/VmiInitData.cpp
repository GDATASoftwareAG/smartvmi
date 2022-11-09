#include "VmiInitData.h"
#include "VmiException.h"
#include <cstring>

// NOLINTBEGIN(cppcoreguidelines-no-malloc)

namespace VmiCore
{
    VmiInitData::VmiInitData(const std::filesystem::path& socketPath)
    {
        if (!socketPath.empty())
        {
            data = reinterpret_cast<vmi_init_data_t*>(malloc(sizeof(vmi_init_data_t) + sizeof(vmi_init_data_entry_t)));

            if (data == nullptr)
            {
                throw VmiException("Unable to allocate memory");
            }

            data->count = 1;
            data->entry[0].type = VMI_INIT_DATA_KVMI_SOCKET;
            data->entry[0].data = strdup(socketPath.c_str());

            if (data->entry[0].data == nullptr)
            {
                free(data);
                throw VmiException("Unable to duplicate string");
            }
        }
    }

    VmiInitData::VmiInitData(VmiInitData&& vmiInitData) noexcept : data(vmiInitData.data)
    {
        vmiInitData.data = nullptr;
    }

    VmiInitData::~VmiInitData()
    {
        if (data != nullptr)
        {
            free(data->entry[0].data); // NOLINT(cppcoreguidelines-owning-memory)
            free(data);
        }
    }
}

// NOLINTEND(cppcoreguidelines-no-malloc)
