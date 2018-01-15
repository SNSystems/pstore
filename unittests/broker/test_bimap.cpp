//*  _     _                        *
//* | |__ (_)_ __ ___   __ _ _ __   *
//* | '_ \| | '_ ` _ \ / _` | '_ \  *
//* | |_) | | | | | | | (_| | |_) | *
//* |_.__/|_|_| |_| |_|\__,_| .__/  *
//*                         |_|     *
//===- unittests/broker/test_bimap.cpp ------------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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

/// \file bimap.hpp

#include "broker/bimap.hpp"
#include <string>
#include "gmock/gmock.h"

TEST (BiMap, Size) {
    bimap<int, char> bm;
    EXPECT_EQ (0U, bm.size ());
    bm.set (1, 'a');
    EXPECT_EQ (1U, bm.size ());
    bm.set (1, 'b');
    EXPECT_EQ (1U, bm.size ());
    bm.getl (2);
    EXPECT_EQ (2U, bm.size ());
    bm.getl (2);
    EXPECT_EQ (2U, bm.size ());
}

TEST (BiMap, SetGet) {
    bimap<std::string, int> bm;
    bm.set ("a", 42);
    bm.set ("b", 37);
    EXPECT_EQ (2U, bm.size ());
    EXPECT_EQ (42, bm.getr ("a"));
    EXPECT_EQ (37, bm.getr ("b"));
    EXPECT_EQ ("a", bm.getl (42));
    EXPECT_EQ ("b", bm.getl (37));
}

TEST (BiMap, Present) {
    bimap<std::string, int> bm;
    bm.set ("a", 42);
    bm.set ("b", 37);
    EXPECT_FALSE (bm.presentl ("false"));
    EXPECT_TRUE (bm.presentl ("a"));
    EXPECT_FALSE (bm.presentr (0));
    EXPECT_TRUE (bm.presentr (42));
}
// eof: unittests/broker/test_bimap.cpp
