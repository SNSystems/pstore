//*  _       _                                          _              *
//* (_) ___ | |_ __ _    __ _  ___ _ __   ___ _ __ __ _| |_ ___  _ __  *
//* | |/ _ \| __/ _` |  / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \| '__| *
//* | | (_) | || (_| | | (_| |  __/ | | |  __/ | | (_| | || (_) | |    *
//* |_|\___/ \__\__,_|  \__, |\___|_| |_|\___|_|  \__,_|\__\___/|_|    *
//*                     |___/                                          *
//===- unittests/broker_poker/test_iota_generator.cpp ---------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "iota_generator.hpp"

#include <gtest/gtest.h>

TEST (IotaGenerator, InitialValue) {
    iota_generator g1;
    EXPECT_EQ (*g1, 0UL);

    iota_generator g2 (3);
    EXPECT_EQ (*g2, 3UL);
}

TEST (IotaGenerator, Compare) {
    iota_generator g1 (3);
    iota_generator g2 (3);
    iota_generator g3 (5);
    EXPECT_EQ (*g1, *g2);
    EXPECT_NE (*g1, *g3);
}

TEST (IotaGenerator, Increment) {
    iota_generator g1 (3);
    iota_generator post = g1++;
    EXPECT_EQ (post, iota_generator (3));
    EXPECT_EQ (g1, iota_generator (4));

    iota_generator pre = ++g1;
    EXPECT_EQ (pre, iota_generator (5));
    EXPECT_EQ (g1, iota_generator (5));
}
