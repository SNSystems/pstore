//*                      _      _ _ _ _      *
//*   _____  ____  _____| |_ __| | (_) |__   *
//*  / __\ \/ /\ \/ / __| __/ _` | | | '_ \  *
//* | (__ >  <  >  <\__ \ || (_| | | | |_) | *
//*  \___/_/\_\/_/\_\___/\__\__,_|_|_|_.__/  *
//*                                          *
//===- unittests/klee/cxxstdlib.cpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cstdlib>
#include <string>
#include <system_error>

namespace std {

    // error_category::error_category () noexcept = default;

    error_category::~error_category () = default;

    error_condition error_category::default_error_condition (int ev) const noexcept {
        return error_condition (ev, *this);
    }


    bool error_category::equivalent (int code, error_condition const & condition) const noexcept {
        return default_error_condition (code) == condition;
    }
    bool error_category::equivalent (error_code const & code, int condition) const noexcept {
        return *this == code.category () && code.value () == condition;
    }

    // template <typename T>
    // std::allocator<T>::allocator() noexcept = default;

    template class allocator<char>;
    template class basic_string<char>;

    void terminate () noexcept { std::exit (EXIT_FAILURE); }

} // namespace std
