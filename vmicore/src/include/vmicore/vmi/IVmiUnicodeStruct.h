#ifndef VMICORE_IVMIUNICODESTRUCT_H
#define VMICORE_IVMIUNICODESTRUCT_H

#include <string_view>

namespace VmiCore
{
    class IVmiUnicodeStruct
    {
      public:
        virtual ~IVmiUnicodeStruct() = default;

        virtual operator std::string_view() const = 0;

      protected:
        IVmiUnicodeStruct() = default;
    };
}

#endif // VMICORE_IVMIUNICODESTRUCT_H
