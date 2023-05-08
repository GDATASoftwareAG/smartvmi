#ifndef VMICORE_RUSTHELPER_H
#define VMICORE_RUSTHELPER_H

#include <cstdint>
#include <cxx_rust_part/bridge.h>
#include <fmt/core.h>
#include <initializer_list>
#include <rust/cxx.h>
#include <string_view>
#include <type_traits>

namespace VmiCore
{
    inline ::rust::Str toRustStr(std::string_view stringView)
    {
        return {stringView.data(), stringView.size()};
    }

    // Helper for overload pattern
    template <class... Ts> struct overload : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts> overload(Ts...) -> overload<Ts...>;

    inline void addRustField(::rust::Vec<::logging::LogField>& vec, const CxxLogField& field)
    {
        std::visit(
            overload{[&vec, &field](std::string_view arg)
                     { ::logging::add_field_str(vec, toRustStr(field.first), toRustStr(arg)); },
                     [&vec, &field](bool arg) { ::logging::add_field_bool(vec, toRustStr(field.first), arg); },
                     [&vec, &field](int64_t arg) { ::logging::add_field_i64(vec, toRustStr(field.first), arg); },
                     [&vec, &field](uint64_t arg) { ::logging::add_field_uint64(vec, toRustStr(field.first), arg); },
                     [&vec, &field](double arg) { ::logging::add_field_float64(vec, toRustStr(field.first), arg); }},
            field.second);
    }

    inline void appendCxxFieldsToRustFields(::rust::Vec<::logging::LogField>& rustFields,
                                            const std::initializer_list<CxxLogField>& cxxFields)
    {
        for (const auto& field : cxxFields)
        {
            addRustField(rustFields, field);
        }
    }
}

#endif // VMICORE_RUSTHELPER_H
