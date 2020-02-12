//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- unittests/support/test_error.cpp -----------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/support/error.hpp"
#include <gtest/gtest.h>
#include "check_for_error.hpp"

TEST (Error, None) {
    std::error_code const err = make_error_code (pstore::error_code::none);
    EXPECT_FALSE (err);
    EXPECT_STREQ (err.category ().name (), "pstore category");
    EXPECT_EQ (err.value (), 0);
    EXPECT_EQ (err.message (), "no error");
}

TEST (Error, UnknownRevision) {
    std::error_code const err = make_error_code (pstore::error_code::unknown_revision);
    EXPECT_TRUE (err);
    EXPECT_STREQ (err.category ().name (), "pstore category");
    EXPECT_EQ (err.value (), static_cast<int> (pstore::error_code::unknown_revision));
    EXPECT_EQ (err.message (), "unknown revision");
}

TEST (Error, RaisePstoreError) {
    auto const will_throw = [] () { raise (pstore::error_code::unknown_revision); };
    check_for_error (will_throw, pstore::error_code::unknown_revision);
}

TEST (Error, RaiseErrc) {
    auto const will_throw = [] () { pstore::raise (std::errc::invalid_argument); };
    check_for_error (will_throw, std::errc::invalid_argument);
}
