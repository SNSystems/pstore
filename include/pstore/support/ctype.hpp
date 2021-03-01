//===- include/pstore/support/ctype.hpp -------------------*- mode: C++ -*-===//
//*       _                     *
//*   ___| |_ _   _ _ __   ___  *
//*  / __| __| | | | '_ \ / _ \ *
//* | (__| |_| |_| | |_) |  __/ *
//*  \___|\__|\__, | .__/ \___| *
//*           |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file ctype.hpp

#ifndef PSTORE_SUPPORT_CTYPE_HPP
#define PSTORE_SUPPORT_CTYPE_HPP

#include <cctype>
#include <cwctype>

namespace pstore {

    inline bool isspace (char const c) {
        // std::isspace() has undefined behavior if the input value is not representable as unsigned
        // char and not not equal to EOF.
        auto const uc = static_cast<unsigned char> (c);
        // TODO: std::isspace()'s behavior is affected by the current locale, but our "locale" is
        // always UTF-8.
        return std::isspace (static_cast<int> (uc)) != 0;
    }

    inline bool isspace (wchar_t const c) { return std::iswspace (static_cast<wint_t> (c)) != 0; }

} // namespace pstore

#endif // PSTORE_SUPPORT_CTYPE_HPP
