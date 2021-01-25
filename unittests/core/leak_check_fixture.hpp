//*  _            _           _               _       __ _      _                   *
//* | | ___  __ _| | __   ___| |__   ___  ___| | __  / _(_)_  _| |_ _   _ _ __ ___  *
//* | |/ _ \/ _` | |/ /  / __| '_ \ / _ \/ __| |/ / | |_| \ \/ / __| | | | '__/ _ \ *
//* | |  __/ (_| |   <  | (__| | | |  __/ (__|   <  |  _| |>  <| |_| |_| | | |  __/ *
//* |_|\___|\__,_|_|\_\  \___|_| |_|\___|\___|_|\_\ |_| |_/_/\_\\__|\__,_|_|  \___| *
//*                                                                                 *
//===- unittests/core/leak_check_fixture.hpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef LEAK_CHECK_FIXTURE_HPP
#define LEAK_CHECK_FIXTURE_HPP

#include <UnitTest++.h>
#include "allocator.h"

#ifndef CL_STANDARD_ONLY
#    define CL_STANDARD_ONLY (0)
#endif

#if INSTRUMENT_ALLOCATIONS && !CL_STANDARD_ONLY
class leak_check_fixture {
public:
    leak_check_fixture ()
            : before_ (allocations)
            , before_sequence_ (sequence) {}

    virtual ~leak_check_fixture () {
        dump_allocations_since (NULL, before_sequence_);
        CHECK_EQUAL (before_.number, allocations.number);
        CHECK_EQUAL (before_.total, allocations.total);
    }

private:
    allocation_info before_;
    uint32_t before_sequence_;
};
#else
class leak_check_fixture {
public:
    leak_check_fixture () {}
    virtual ~leak_check_fixture () {}
};
#endif

#endif // LEAK_CHECK_FIXTURE_HPP
