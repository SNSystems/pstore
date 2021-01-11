//*                  _                      _                 _    *
//*  _   _ _ __  ___(_) __ _ _ __   ___  __| |   ___ __ _ ___| |_  *
//* | | | | '_ \/ __| |/ _` | '_ \ / _ \/ _` |  / __/ _` / __| __| *
//* | |_| | | | \__ \ | (_| | | | |  __/ (_| | | (_| (_| \__ \ |_  *
//*  \__,_|_| |_|___/_|\__, |_| |_|\___|\__,_|  \___\__,_|___/\__| *
//*                    |___/                                       *
//===- unittests/support/test_unsigned_cast.cpp ---------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#include "pstore/support/unsigned_cast.hpp"

#include <gtest/gtest.h>

#include "check_for_error.hpp"

TEST (UnsignedCast, Zero) {
    EXPECT_EQ (pstore::checked_unsigned_cast (0), 0U);
}

TEST (UnsignedCast, Max) {
    auto const max = std::numeric_limits<int>::max ();
    EXPECT_EQ (pstore::checked_unsigned_cast (max), static_cast<unsigned> (max));
}

TEST (UnsignedCast, Negative) {
    check_for_error ([] () { pstore::checked_unsigned_cast (-1); }, std::errc::invalid_argument);
}
