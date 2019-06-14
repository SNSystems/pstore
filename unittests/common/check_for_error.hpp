//*       _               _       __                                        *
//*   ___| |__   ___  ___| | __  / _| ___  _ __    ___ _ __ _ __ ___  _ __  *
//*  / __| '_ \ / _ \/ __| |/ / | |_ / _ \| '__|  / _ \ '__| '__/ _ \| '__| *
//* | (__| | | |  __/ (__|   <  |  _| (_) | |    |  __/ |  | | | (_) | |    *
//*  \___|_| |_|\___|\___|_|\_\ |_|  \___/|_|     \___|_|  |_|  \___/|_|    *
//*                                                                         *
//===- unittests/common/check_for_error.hpp -------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
#if PSTORE_EXCEPTIONS
    auto f = [&]() {
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
