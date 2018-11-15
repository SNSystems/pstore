//*                        _           *
//*  _ __ ___   __ _ _   _| |__   ___  *
//* | '_ ` _ \ / _` | | | | '_ \ / _ \ *
//* | | | | | | (_| | |_| | |_) |  __/ *
//* |_| |_| |_|\__,_|\__, |_.__/ \___| *
//*                  |___/             *
//===- unittests/support/test_maybe.cpp -----------------------------------===//
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

#include "pstore/support/maybe.hpp"
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include "pstore/support/make_unique.hpp"

using pstore::just;
using pstore::maybe;
using pstore::nothing;

namespace {

    class value {
    public:
        value (int v)
                : v_{std::make_unique<int> (v)} {}
        value (value const & v)
                : v_{std::make_unique<int> (v.get ())} {}

        value & operator= (value const & rhs) {
            v_ = std::make_unique<int> (rhs.get ());
            return *this;
        }

        bool operator== (value const & rhs) const { return this->get () == rhs.get (); }

        int get () const { return *v_; }

    private:
        std::unique_ptr<int> v_;
    };

} // namespace

TEST (Maybe, NoValue) {
    maybe<value> m;
    EXPECT_FALSE (m.operator bool ());
    EXPECT_FALSE (m.has_value ());
    EXPECT_FALSE (m);
}

TEST (Maybe, Nothing) {
    EXPECT_EQ (nothing<value> (), maybe<value> ());
}

TEST (Maybe, Just) {
    EXPECT_EQ (just (value (37)), maybe<value> (37));
}

TEST (Maybe, Value) {
    maybe<value> m (42);
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), value (42));

    m.reset ();
    EXPECT_FALSE (m.has_value ());
}

TEST (Maybe, CtorWithMaybeHoldingAValue) {
    maybe<value> m1 (42);
    maybe<value> m2 = m1;
    EXPECT_TRUE (m2.has_value ());
    EXPECT_TRUE (m2);
    EXPECT_EQ (m2.value (), value (42));
}

TEST (Maybe, ValueOr) {
    maybe<value> m1;
    EXPECT_EQ (m1.value_or (37), 37);

    maybe<value> m2 (5);
    EXPECT_EQ (m2.value_or (37), 5);
}

TEST (Maybe, AssignValue) {
    maybe<value> m;

    // First assignment, m has no value
    m.operator= (43);
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), 43);

    // Second assignment, m holds a value
    m.operator= (44);
    EXPECT_TRUE (m.has_value ());
    EXPECT_TRUE (m);
    EXPECT_EQ (m.value (), 44);

    // Third assignment, m holds a value, assigning nothing.
    m.operator= (nothing<value> ());
    EXPECT_FALSE (m.has_value ());
}

TEST (Maybe, AssignZero) {
    maybe<char> m;
    m = char{0};
    EXPECT_TRUE (m.operator bool ());
    EXPECT_TRUE (m.has_value ());
    EXPECT_EQ (m.value (), 0);
    EXPECT_EQ (*m, 0);

    maybe<char> m2 = m;
    EXPECT_TRUE (m2.has_value ());
    EXPECT_EQ (m2.value (), 0);
    EXPECT_EQ (*m2, 0);
}

TEST (Maybe, MoveCtor) {
    {
        maybe<std::string> m1{std::string{"test"}};
        EXPECT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), "test");

        maybe<std::string> m2 = std::move (m1);
        EXPECT_TRUE (m2.has_value ());
        EXPECT_EQ (*m2, "test");
    }
    {
        // Now move to a maybe<> with no initial value.
        maybe<std::string> m3;
        maybe<std::string> m4 = std::move (m3);
        EXPECT_FALSE (m4.has_value ());
    }
}

TEST (Maybe, MoveAssign) {
    // No initial value
    {
        maybe<std::string> m1;
        EXPECT_FALSE (m1);
        m1 = std::string{"test"};
        EXPECT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }
    // With an initial value
    {
        maybe<std::string> m2{"before"};
        EXPECT_TRUE (m2);
        maybe<std::string> m3{"after"};
        EXPECT_TRUE (m2);

        m2 = std::move (m3);
        EXPECT_TRUE (m2.has_value ());
        EXPECT_TRUE (m3.has_value ()); // a moved-from maybe still contains a value.
        EXPECT_EQ (m2.value (), std::string{"after"});
    }
}

TEST (Maybe, CopyAssign) {
    // both lhs and rhs have no value.
    {
        maybe<std::string> m1;
        maybe<std::string> const m2;
        m1.operator= (m2);
        EXPECT_FALSE (m1.has_value ());
    }

    // lhs with no value, rhs with value.
    {
        maybe<std::string> m1;
        maybe<std::string> const m2 ("test");
        m1.operator= (m2);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }

    // lhs with value, rhs with no value.
    {
        maybe<std::string> m1 ("test");
        maybe<std::string> m2;
        m1.operator= (m2);
        EXPECT_FALSE (m1.has_value ());
    }

    // both lhs and rhs have a value.
    {
        maybe<std::string> m1 ("original");
        maybe<std::string> m2 ("new");
        m1.operator= (m2);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"new"});
    }
}

TEST (Maybe, SelfAssign) {
    // self-assign with no value
    {
        maybe<std::string> m1;
        m1.operator= (m1);
        EXPECT_FALSE (m1.has_value ());
    }

    // self-assign with a value
    {
        maybe<std::string> m1 ("test");
        m1.operator= (m1);
        ASSERT_TRUE (m1.has_value ());
        EXPECT_EQ (m1.value (), std::string{"test"});
    }
}
