//*  _                             _   _              *
//* | |__  ___ ___   ___  ___  ___| |_(_) ___  _ __   *
//* | '_ \/ __/ __| / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | |_) \__ \__ \ \__ \  __/ (__| |_| | (_) | | | | *
//* |_.__/|___/___/ |___/\___|\___|\__|_|\___/|_| |_| *
//*                                                   *
//===- unittests/mcrepo/test_bss_section.cpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/bss_section.hpp"

#include <gmock/gmock.h>

TEST (BssSection, Ctor) {
    pstore::repo::bss_section bss{2, 16};
    EXPECT_EQ (bss.align (), 2U);
    EXPECT_EQ (bss.size (), 16U);
    EXPECT_EQ (bss.ifixups ().size (), 0U);
}
