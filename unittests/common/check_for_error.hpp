//===- unittests/common/check_for_error.hpp ---------------*- mode: C++ -*-===//
//*       _               _       __                                        *
//*   ___| |__   ___  ___| | __  / _| ___  _ __    ___ _ __ _ __ ___  _ __  *
//*  / __| '_ \ / _ \/ __| |/ / | |_ / _ \| '__|  / _ \ '__| '__/ _ \| '__| *
//* | (__| | | |  __/ (__|   <  |  _| (_) | |    |  __/ |  | | | (_) | |    *
//*  \___|_| |_|\___|\___|_|\_\ |_|  \___/|_|     \___|_|  |_|  \___/|_|    *
//*                                                                         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CHECK_FOR_ERROR_HPP
#define CHECK_FOR_ERROR_HPP

#include <system_error>

#include <gtest/gtest.h>

#include "pstore/config/config.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"

template <typename Function>
void check_for_error (Function test_fn, int err, std::error_category const & category) {
#ifdef PSTORE_EXCEPTIONS
    auto f = [&] () {
        try {
            test_fn ();
        } catch (std::system_error const & ex) {
            EXPECT_EQ (category, ex.code ().category ());
            EXPECT_EQ (err, ex.code ().value ());
            throw;
        }
    };
    EXPECT_THROW (f (), std::system_error);
#else
    (void) test_fn;
    (void) err;
    (void) category;
#endif // PSTORE_EXCEPTIONS
}

template <typename Function>
void check_for_error (Function fn, pstore::error_code err) {
    check_for_error (fn, static_cast<int> (err), pstore::get_error_category ());
}

template <typename Function>
void check_for_error (Function fn, std::errc err) {
    check_for_error (fn, static_cast<int> (err), std::generic_category ());
}

template <typename Function>
void check_for_error (Function fn, pstore::errno_erc err) {
    check_for_error (fn, err.get (), std::generic_category ());
}

#ifdef _WIN32
template <typename Function>
void check_for_error (Function fn, pstore::win32_erc err) {
    check_for_error (fn, err.get (), std::system_category ());
}
#endif //_WIN32

#endif // CHECK_FOR_ERROR_HPP
