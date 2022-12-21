#ifndef VMICORE_VMIUNICODESTRUCT_H
#define VMICORE_VMIUNICODESTRUCT_H

#include <libvmi/libvmi.h>
#include <string_view>
#include <vmicore/vmi/IVmiUnicodeStruct.h>

namespace VmiCore
{
    class VmiUnicodeStruct : public IVmiUnicodeStruct
    {
      public:
        VmiUnicodeStruct() = default;

        VmiUnicodeStruct(const VmiUnicodeStruct&) = delete;

        VmiUnicodeStruct(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept;

        ~VmiUnicodeStruct() override;

        VmiUnicodeStruct& operator=(VmiUnicodeStruct&) = delete;

        VmiUnicodeStruct& operator=(VmiUnicodeStruct&& vmiUnicodeStruct) noexcept;

        bool operator==(const VmiUnicodeStruct& rhs) const;

        unicode_string_t* data();

        operator std::string_view() const override;

      private:
        unicode_string_t unicodeData{};
    };
}

#endif // VMICORE_VMIUNICODESTRUCT_H
