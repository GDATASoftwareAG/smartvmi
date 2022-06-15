#include "VmiUnicodeStruct.h"
#include "VmiException.h"
#include <cstdlib>
#include <cstring>

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

std::string_view VmiUnicodeStruct::asStringView() const
{
    if (unicodeData.contents == nullptr)
    {
        throw VmiException("No unicode string provided");
    }
    return {reinterpret_cast<char*>(unicodeData.contents), unicodeData.length};
}

std::string_view VmiUnicodeStruct::asStringViewOrDefault() const
{
    if (unicodeData.contents == nullptr)
    {
        return {};
    }
    return {reinterpret_cast<char*>(unicodeData.contents), unicodeData.length};
}
