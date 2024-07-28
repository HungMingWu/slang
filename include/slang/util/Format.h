//------------------------------------------------------------------------------
//! @file Format.h
//! @brief Wrapper for C++ format and extension
//
// SPDX-FileCopyrightText: Michael Popoloski
// SPDX-License-Identifier: MIT
//------------------------------------------------------------------------------

#pragma once

#include <format>

namespace slang {
    using std::format;

    template <typename Char = char>
    struct runtime_format_string {
        std::basic_string_view<Char> str;
    };

    inline auto runtime(std::string_view s) -> runtime_format_string<> {
        return {{s}};
    }

    template<typename Char, typename... T>
    inline std::string format(runtime_format_string<Char> fmt, T&&... args) {
        return std::vformat(fmt.str, std::make_format_args(args...));
    }
}
