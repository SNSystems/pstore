//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===- unittests/command_line/test_str_to_revision.cpp --------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/command_line/str_to_revision.hpp"

// Standard Library
#include <utility>

// 3rd party
#include <gmock/gmock.h>

// pstore
#include "pstore/support/head_revision.hpp"

TEST (StrToRevision, SingleCharacterNumber) {
    EXPECT_EQ (pstore::str_to_revision ("1"), pstore::just (1U));
}

TEST (StrToRevision, MultiCharacterNumber) {
    EXPECT_EQ (pstore::str_to_revision ("200000"), pstore::just (200000U));
}

TEST (StrToRevision, NumberLeadingWS) {
    EXPECT_EQ (pstore::str_to_revision ("    200000"), pstore::just (200000U));
}

TEST (StrToRevision, NumberTrailingWS) {
    EXPECT_EQ (pstore::str_to_revision ("12345   "), pstore::just (12345U));
}

TEST (StrToRevision, Empty) {
    EXPECT_EQ (pstore::str_to_revision (""), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, JustWhitespace) {
    EXPECT_EQ (pstore::str_to_revision ("  \t"), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, Zero) {
    EXPECT_EQ (pstore::str_to_revision ("0"), pstore::just (0U));
}

TEST (StrToRevision, HeadLowerCase) {
    EXPECT_EQ (pstore::str_to_revision ("head"), pstore::just (pstore::head_revision));
}

TEST (StrToRevision, HeadMixedCase) {
    EXPECT_EQ (pstore::str_to_revision ("HeAd"), pstore::just (pstore::head_revision));
}

TEST (StrToRevision, HeadLeadingWhitespace) {
    EXPECT_EQ (pstore::str_to_revision ("  HEAD"), pstore::just (pstore::head_revision));
}

TEST (StrToRevision, HeadTraingWhitespace) {
    EXPECT_EQ (pstore::str_to_revision ("HEAD  "), pstore::just (pstore::head_revision));
}

TEST (StrToRevision, BadString) {
    EXPECT_EQ (pstore::str_to_revision ("bad"), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, NumberFollowedByString) {
    EXPECT_EQ (pstore::str_to_revision ("123Bad"), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, PositiveOverflow) {
    std::ostringstream str;
    str << std::numeric_limits<unsigned>::max () + 1ULL;
    EXPECT_EQ (pstore::str_to_revision (str.str ()), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, Negative) {
    EXPECT_EQ (pstore::str_to_revision ("-2"), pstore::nothing<unsigned> ());
}

TEST (StrToRevision, Hex) {
    EXPECT_EQ (pstore::str_to_revision ("0x23"), pstore::nothing<unsigned> ());
}
