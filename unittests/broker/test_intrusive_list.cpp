//*  _       _                  _             _ _     _    *
//* (_)_ __ | |_ _ __ _   _ ___(_)_   _____  | (_)___| |_  *
//* | | '_ \| __| '__| | | / __| \ \ / / _ \ | | / __| __| *
//* | | | | | |_| |  | |_| \__ \ |\ V /  __/ | | \__ \ |_  *
//* |_|_| |_|\__|_|   \__,_|___/_| \_/ \___| |_|_|___/\__| *
//*                                                        *
//===- unittests/broker/test_intrusive_list.cpp ---------------------------===//
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
#include "broker/intrusive_list.hpp"
#include <gtest/gtest.h>

namespace {
    struct value {
        value ()
                : v{0} {}
        explicit value (int v_)
                : v{v_} {}
        list_member<value> & get_list_member () {
            return list_memb;
        }

        int const v;
        list_member<value> list_memb;
    };
}

TEST (IntrusiveList, Empty) {
    intrusive_list<value> v;
    EXPECT_EQ (0, std::distance (v.begin (), v.end ()));
}

TEST (IntrusiveList, OneElement) {
    value member{47};

    intrusive_list<value> v;
    v.insert_before (&member, v.tail ());

    ASSERT_EQ (1, std::distance (v.begin (), v.end ()));
    EXPECT_EQ (v.begin ()->v, 47);

    v.erase (&member);
    EXPECT_EQ (0, std::distance (v.begin (), v.end ()));
    EXPECT_EQ (v.begin (), v.end ());
}

TEST (IntrusiveList, IteratorIncrement) {
    value member{7};
    intrusive_list<value> v;
    v.insert_before (&member, v.tail ());

    intrusive_list<value>::iterator begin = v.begin ();
    intrusive_list<value>::iterator it = begin;
    ++it;
    intrusive_list<value>::iterator it2 = begin;
    it2++;
    EXPECT_EQ (it, it2);

    --it;
    it2--;
    EXPECT_EQ (it, it2);
    EXPECT_EQ (it, begin);
}
// eof: unittests/broker/test_intrusive_list.cpp
