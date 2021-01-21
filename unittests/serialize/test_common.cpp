//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- unittests/serialize/test_common.cpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "pstore/serialize/common.hpp"
#include "gmock/gmock.h"

TEST (StickyAssign, Construction) {
    pstore::serialize::sticky_assign<int> default_init;
    EXPECT_EQ (0, default_init.get ());

    pstore::serialize::sticky_assign<int> explicit_init (32);
    EXPECT_EQ (32, explicit_init.get ());
}

TEST (StickyAssign, FirstAssignmentAfterDefaultConstruction) {
    pstore::serialize::sticky_assign<int> v;
    v = 73;
    EXPECT_EQ (73, v.get ());
}

TEST (StickyAssign, TwoAssignmentsAfterDefaultConstruction) {
    pstore::serialize::sticky_assign<int> v;
    v = 17;
    v = 42;
    EXPECT_EQ (17, v.get ());
}

TEST (StickyAssign, FirstAssignmentAfterInitialization) {
    pstore::serialize::sticky_assign<int> v (17);
    v = 73;
    EXPECT_EQ (17, v.get ());
}

TEST (StickyAssign, Copy) {
    pstore::serialize::sticky_assign<int> v1;      // default construct v1.
    pstore::serialize::sticky_assign<int> v2 (v1); // value construct v2.
    v2 = 42;                                       // assignment to v2 is ignored.
    EXPECT_EQ (0, v2.get ());
}
