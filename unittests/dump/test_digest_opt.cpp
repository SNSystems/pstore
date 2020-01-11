//*      _ _                 _                 _    *
//*   __| (_) __ _  ___  ___| |_    ___  _ __ | |_  *
//*  / _` | |/ _` |/ _ \/ __| __|  / _ \| '_ \| __| *
//* | (_| | | (_| |  __/\__ \ |_  | (_) | |_) | |_  *
//*  \__,_|_|\__, |\___||___/\__|  \___/| .__/ \__| *
//*          |___/                      |_|         *
//===- unittests/dump/test_digest_opt.cpp ---------------------------------===//
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
#include "pstore/dump/digest_opt.hpp"
#include <gtest/gtest.h>

TEST (DigestOpt, Empty) {
    EXPECT_EQ (pstore::dump::digest_from_string (""), pstore::nothing<pstore::index::digest> ());
}

TEST (DigestOpt, Bad) {
    EXPECT_EQ (pstore::dump::digest_from_string ("0000000000000000000000000000000g"),
               pstore::nothing<pstore::index::digest> ());
}

TEST (DigestOpt, Digits) {
    EXPECT_EQ (pstore::dump::digest_from_string ("00000000000000000000000000000000"),
               pstore::just (pstore::index::digest{0U}));
    EXPECT_EQ (pstore::dump::digest_from_string ("00000000000000000000000000000001"),
               pstore::just (pstore::index::digest{1U}));
    EXPECT_EQ (pstore::dump::digest_from_string ("10000000000000000000000000000001"),
               pstore::just (pstore::index::digest{0x10000000'00000000U, 0x00000000'00000001U}));
    EXPECT_EQ (pstore::dump::digest_from_string ("99999999999999999999999999999999"),
               pstore::just (pstore::index::digest{0x99999999'99999999U, 0x99999999'99999999U}));
}
TEST (DigestOpt, Alpha) {
    EXPECT_EQ (pstore::dump::digest_from_string ("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"),
               pstore::just (pstore::index::digest{0xFFFFFFFF'FFFFFFFFU, 0xFFFFFFFF'FFFFFFFFU}));
    EXPECT_EQ (pstore::dump::digest_from_string ("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"),
               pstore::just (pstore::index::digest{0xAAAAAAAA'AAAAAAAAU, 0xAAAAAAAA'AAAAAAAAU}));
    EXPECT_EQ (pstore::dump::digest_from_string ("ffffffffffffffffffffffffffffffff"),
               pstore::just (pstore::index::digest{0xFFFFFFFF'FFFFFFFFU, 0xFFFFFFFF'FFFFFFFFU}));
    EXPECT_EQ (pstore::dump::digest_from_string ("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
               pstore::just (pstore::index::digest{0xAAAAAAAA'AAAAAAAAU, 0xAAAAAAAA'AAAAAAAAU}));
}
