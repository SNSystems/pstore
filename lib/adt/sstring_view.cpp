//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- lib/adt/sstring_view.cpp -------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/adt/sstring_view.hpp"

#include <algorithm>

namespace pstore {

    sstring_view<std::shared_ptr<char const>> make_shared_sstring_view (std::string const & str) {
        auto const length = str.length ();
        auto const ptr =
            std::shared_ptr<char> (new char[length], [] (gsl::czstring const p) { delete[] p; });
        std::copy (std::begin (str), std::end (str), ptr.get ());
        return pstore::make_shared_sstring_view (std::static_pointer_cast<char const> (ptr),
                                                 length);
    }

    sstring_view<std::shared_ptr<char const>> make_shared_sstring_view (gsl::czstring const str) {
        auto const length = std::strlen (str);
        auto const ptr =
            std::shared_ptr<char> (new char[length], [] (gsl::czstring const p) { delete[] p; });
        std::copy (str, str + length, ptr.get ());
        return pstore::make_shared_sstring_view (std::static_pointer_cast<char const> (ptr),
                                                 length);
    }

    sstring_view<char const *> make_sstring_view (gsl::czstring const str) {
        return {str, std::strlen (str)};
    }

} // end namespace pstore
