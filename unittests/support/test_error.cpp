//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- unittests/support/test_error.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
