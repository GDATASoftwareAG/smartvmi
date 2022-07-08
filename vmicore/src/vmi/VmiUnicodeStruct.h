#ifndef VMICORE_VMIUNICODESTRUCT_H
#define VMICORE_VMIUNICODESTRUCT_H

#include <libvmi/libvmi.h>
#include <string_view>

class VmiUnicodeStruct
{
  public:
    VmiUnicodeStruct() = default;

    VmiUnicodeStruct(const VmiUnicodeStruct&) = delete;

    VmiUnicodeStruct(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept;

    ~VmiUnicodeStruct();

    VmiUnicodeStruct& operator=(VmiUnicodeStruct&) = delete;

    VmiUnicodeStruct& operator=(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept;

    bool operator==(const VmiUnicodeStruct& rhs) const;

    unicode_string_t* data();

    [[nodiscard]] std::string_view asStringView() const;

    [[nodiscard]] std::string_view asStringViewOrDefault() const;

  private:
    unicode_string_t unicodeData{};
};

#endif // VMICORE_VMIUNICODESTRUCT_H
