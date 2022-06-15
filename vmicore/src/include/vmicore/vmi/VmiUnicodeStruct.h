#ifndef VMICORE_VMIUNICODESTRUCT_H
#define VMICORE_VMIUNICODESTRUCT_H

#include <cstdlib>
#include <cstring>
#include <libvmi/libvmi.h>
#include <stdexcept>
#include <string_view>

namespace VmiCore
{
    class VmiUnicodeStruct
    {
      public:
        VmiUnicodeStruct() = default;

        VmiUnicodeStruct(const VmiUnicodeStruct&) = delete;

        VmiUnicodeStruct(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept : unicodeData(vmiUnicodeStruct.unicodeData)
        {
            vmiUnicodeStruct.unicodeData.contents = nullptr;
        }

        ~VmiUnicodeStruct()
        {
            if (unicodeData.contents != nullptr)
            {
                free(unicodeData.contents); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            }
        }

        VmiUnicodeStruct& operator=(VmiUnicodeStruct&) = delete;

        VmiUnicodeStruct& operator=(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept
        {
            unicodeData = vmiUnicodeStruct.unicodeData;
            vmiUnicodeStruct.unicodeData.contents = nullptr;
            return *this;
        }

        bool operator==(const VmiUnicodeStruct& rhs) const
        {
            return (std::strcmp(reinterpret_cast<const char*>(unicodeData.contents),
                                reinterpret_cast<const char*>(rhs.unicodeData.contents)))
                       ? false
                   : (unicodeData.length != rhs.unicodeData.length)     ? false
                   : (unicodeData.encoding != rhs.unicodeData.encoding) ? false
                                                                        : true;
        }

        inline unicode_string_t* data()
        {
            return &unicodeData;
        }

        operator std::string_view() const
        {
            if (unicodeData.contents == nullptr)
            {
                return {};
            }
            return {reinterpret_cast<char*>(unicodeData.contents), unicodeData.length};
        }

      private:
        unicode_string_t unicodeData{};
    };
}

#endif // VMICORE_VMIUNICODESTRUCT_H
