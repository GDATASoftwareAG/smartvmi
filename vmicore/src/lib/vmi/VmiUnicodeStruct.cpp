#include "VmiUnicodeStruct.h"
#include <cstdlib>
#include <cstring>

namespace VmiCore
{

    VmiUnicodeStruct::VmiUnicodeStruct(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept
        : unicodeData(vmiUnicodeStruct.unicodeData)
    {
        vmiUnicodeStruct.unicodeData.contents = nullptr;
    }

    VmiUnicodeStruct::~VmiUnicodeStruct()
    {
        if (unicodeData.contents != nullptr)
        {
            free(unicodeData.contents); // NOLINT(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        }
    }

    VmiUnicodeStruct& VmiUnicodeStruct::operator=(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept
    {
        unicodeData = vmiUnicodeStruct.unicodeData;
        vmiUnicodeStruct.unicodeData.contents = nullptr;
        return *this;
    }

    bool VmiUnicodeStruct::operator==(const VmiUnicodeStruct& rhs) const
    {
        return (std::strcmp(reinterpret_cast<const char*>(unicodeData.contents),
                            reinterpret_cast<const char*>(rhs.unicodeData.contents)))
                   ? false
               : (unicodeData.length != rhs.unicodeData.length)     ? false
               : (unicodeData.encoding != rhs.unicodeData.encoding) ? false
                                                                    : true;
    }

    unicode_string_t* VmiUnicodeStruct::data()
    {
        return &unicodeData;
    }

    VmiUnicodeStruct::operator std::string_view() const
    {
        if (unicodeData.contents == nullptr)
        {
            return {};
        }
        return {reinterpret_cast<char*>(unicodeData.contents), unicodeData.length};
    }
}
