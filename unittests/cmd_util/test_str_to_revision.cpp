//*      _          _                         _     _              *
//*  ___| |_ _ __  | |_ ___    _ __ _____   _(_)___(_) ___  _ __   *
//* / __| __| '__| | __/ _ \  | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* \__ \ |_| |    | || (_) | | | |  __/\ V /| \__ \ | (_) | | | | *
//* |___/\__|_|     \__\___/  |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                                                *
//===- unittests/cmd_util/test_str_to_revision.cpp ------------------------===//
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

#include "pstore/cmd_util/str_to_revision.hpp"

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
