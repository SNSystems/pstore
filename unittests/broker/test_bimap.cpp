//===- unittests/broker/test_bimap.cpp ------------------------------------===//
//*  _     _                        *
//* | |__ (_)_ __ ___   __ _ _ __   *
//* | '_ \| | '_ ` _ \ / _` | '_ \  *
//* | |_) | | | | | | | (_| | |_) | *
//* |_.__/|_|_| |_| |_|\__,_| .__/  *
//*                         |_|     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// \file bimap.hpp

#include "pstore/broker/bimap.hpp"
#include <string>
#include "gmock/gmock.h"

TEST (BiMap, Size) {
    pstore::broker::bimap<int, char> bm;
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
    pstore::broker::bimap<std::string, int> bm;
    bm.set ("a", 42);
    bm.set ("b", 37);
    EXPECT_EQ (2U, bm.size ());
    EXPECT_EQ (42, bm.getr ("a"));
    EXPECT_EQ (37, bm.getr ("b"));
    EXPECT_EQ ("a", bm.getl (42));
    EXPECT_EQ ("b", bm.getl (37));
}

TEST (BiMap, Present) {
    pstore::broker::bimap<std::string, int> bm;
    bm.set ("a", 42);
    bm.set ("b", 37);
    EXPECT_FALSE (bm.presentl ("false"));
    EXPECT_TRUE (bm.presentl ("a"));
    EXPECT_FALSE (bm.presentr (0));
    EXPECT_TRUE (bm.presentr (42));
}
