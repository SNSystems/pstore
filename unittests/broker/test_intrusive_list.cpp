//*  _       _                  _             _ _     _    *
//* (_)_ __ | |_ _ __ _   _ ___(_)_   _____  | (_)___| |_  *
//* | | '_ \| __| '__| | | / __| \ \ / / _ \ | | / __| __| *
//* | | | | | |_| |  | |_| \__ \ |\ V /  __/ | | \__ \ |_  *
//* |_|_| |_|\__|_|   \__,_|___/_| \_/ \___| |_|_|___/\__| *
//*                                                        *
//===- unittests/broker/test_intrusive_list.cpp ---------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/broker/intrusive_list.hpp"
#include <gtest/gtest.h>

namespace {
    struct value {
        value ()
                : v{0} {}
        explicit value (int v_)
                : v{v_} {}
        pstore::broker::list_member<value> & get_list_member () noexcept { return list_memb; }

        int const v;
        pstore::broker::list_member<value> list_memb;
    };
} // namespace

TEST (IntrusiveList, Empty) {
    pstore::broker::intrusive_list<value> v;
    EXPECT_EQ (0, std::distance (v.begin (), v.end ()));
}

TEST (IntrusiveList, OneElement) {
    value member{47};

    pstore::broker::intrusive_list<value> v;
    v.insert_before (&member, v.tail ());

    ASSERT_EQ (1, std::distance (v.begin (), v.end ()));
    EXPECT_EQ (v.begin ()->v, 47);

    v.erase (&member);
    EXPECT_EQ (0, std::distance (v.begin (), v.end ()));
    EXPECT_EQ (v.begin (), v.end ());
}

TEST (IntrusiveList, IteratorIncrement) {
    value member{7};
    pstore::broker::intrusive_list<value> v;
    v.insert_before (&member, v.tail ());

    pstore::broker::intrusive_list<value>::iterator begin = v.begin ();
    pstore::broker::intrusive_list<value>::iterator it = begin;
    ++it;
    pstore::broker::intrusive_list<value>::iterator it2 = begin;
    it2++;
    EXPECT_EQ (it, it2);

    --it;
    it2--;
    EXPECT_EQ (it, it2);
    EXPECT_EQ (it, begin);
}
