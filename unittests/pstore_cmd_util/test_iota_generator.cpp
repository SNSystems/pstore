//*  _       _                                          _              *
//* (_) ___ | |_ __ _    __ _  ___ _ __   ___ _ __ __ _| |_ ___  _ __  *
//* | |/ _ \| __/ _` |  / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \| '__| *
//* | | (_) | || (_| | | (_| |  __/ | | |  __/ | | (_| | || (_) | |    *
//* |_|\___/ \__\__,_|  \__, |\___|_| |_|\___|_|  \__,_|\__\___/|_|    *
//*                     |___/                                          *
//===- unittests/pstore_cmd_util/test_iota_generator.cpp ------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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

#include "pstore_cmd_util/iota_generator.hpp"
#include <gtest/gtest.h>

TEST (IotaGenerator, InitialValue) {
    pstore::cmd_util::iota_generator g1;
    EXPECT_EQ (*g1, 0UL);

    pstore::cmd_util::iota_generator g2 (3);
    EXPECT_EQ (*g2, 3UL);
}

TEST (IotaGenerator, Compare) {
    pstore::cmd_util::iota_generator g1 (3);
    pstore::cmd_util::iota_generator g2 (3);
    pstore::cmd_util::iota_generator g3 (5);
    EXPECT_EQ (*g1, *g2);
    EXPECT_NE (*g1, *g3);
}

TEST (IotaGenerator, Increment) {
    pstore::cmd_util::iota_generator g1 (3);
    pstore::cmd_util::iota_generator post = g1++;
    EXPECT_EQ (post, pstore::cmd_util::iota_generator (3));
    EXPECT_EQ (g1, pstore::cmd_util::iota_generator (4));

    pstore::cmd_util::iota_generator pre = ++g1;
    EXPECT_EQ (pre, pstore::cmd_util::iota_generator (5));
    EXPECT_EQ (g1, pstore::cmd_util::iota_generator (5));
}
// eof: unittests/pstore_cmd_util/test_iota_generator.cpp
