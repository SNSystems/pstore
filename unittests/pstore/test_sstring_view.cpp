//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- unittests/pstore/test_sstring_view.cpp -----------------------------===//
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

#include "pstore/sstring_view.hpp"
#include <gmock/gmock.h>

namespace {
    pstore::sstring_view new_sstring_view (char const * s) {
        auto const length = pstore::sstring_view_details::length (s);
        auto const data = pstore::sstring_view_details::data (s);
        auto ptr = std::shared_ptr<char> (new char[length], [](char * p) { delete[] p; });
        std::copy (data, data + length, ptr.get ());
        return {ptr, length};
    }

    class SStringView : public ::testing::Test {
    protected:
        std::shared_ptr<char> new_shared (std::string const & s) {
            auto result = std::shared_ptr<char> (new char[s.size ()], [](char * p) { delete[] p; });
            std::copy (std::begin (s), std::end (s), result.get ());
            return result;
        }
    };
}
TEST_F (SStringView, Init) {
    pstore::sstring_view sv;
    EXPECT_EQ (sv.size (), 0U);
    EXPECT_EQ (sv.length (), 0U);
    EXPECT_EQ (sv.max_size (), 0U);
    EXPECT_TRUE (sv.empty ());
    EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 0);
}

TEST_F (SStringView, At) {
    std::string const src{"ABCDE"};
    pstore::sstring_view const sv = new_sstring_view (src.c_str ());
    ASSERT_EQ (sv.length (), src.length ());
    EXPECT_FALSE (sv.empty ());
    EXPECT_EQ (sv[0], sv.at (0));
    EXPECT_EQ (sv[1], sv.at (1));
    EXPECT_EQ (sv[4], sv.at (4));
#if PSTORE_CPP_EXCEPTIONS
    EXPECT_THROW (sv.at (5), std::out_of_range);
#endif
}

TEST_F (SStringView, Back) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.back (), src[length - 1]);
    EXPECT_EQ (&sv.back (), ptr.get () + length - 1);
}

TEST_F (SStringView, Data) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.data (), ptr.get ());
}

TEST_F (SStringView, Front) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.front (), src[0]);
    EXPECT_EQ (&sv.front (), ptr.get ());
}

TEST_F (SStringView, Index) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv (ptr, length);

    EXPECT_EQ (sv[0], src[0]);
    EXPECT_EQ (&sv[0], ptr.get () + 0);
    EXPECT_EQ (sv[1], src[1]);
    EXPECT_EQ (&sv[1], ptr.get () + 1);
    EXPECT_EQ (sv[4], src[4]);
    EXPECT_EQ (&sv[4], ptr.get () + 4);
}

namespace {
    template <typename StringType>
    class SStringViewRelational : public SStringView {};

    class sstringview_maker {
    public:
        sstringview_maker (char const * s)
                : view_ (new_sstring_view (s)) {}
        operator pstore::sstring_view () const {
            return view_;
        }

    private:
        pstore::sstring_view view_;
    };

    using StringTypes = ::testing::Types<sstringview_maker, char const *, std::string>;
}

TYPED_TEST_CASE (SStringViewRelational, StringTypes);

TYPED_TEST (SStringViewRelational, Eq) {
#define EQ(lhs, rhs, x)                                                                            \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view (lhs);                                    \
        EXPECT_EQ (lhs_view == (rhs), (x));                                                        \
        EXPECT_EQ ((rhs) == lhs_view, (x));                                                        \
    }
    EQ ("", TypeParam (""), true);
    EQ ("", TypeParam ("abcde"), false);
    EQ ("", TypeParam ("abcdefghij"), false);
    EQ ("", TypeParam ("abcdefghijklmnopqrst"), false);
    EQ ("abcde", TypeParam (""), false);
    EQ ("abcde", TypeParam ("abcde"), true);
    EQ ("abcde", TypeParam ("abcdefghij"), false);
    EQ ("abcde", TypeParam ("abcdefghijklmnopqrst"), false);
    EQ ("abcdefghij", TypeParam (""), false);
    EQ ("abcdefghij", TypeParam ("abcde"), false);
    EQ ("abcdefghij", TypeParam ("abcdefghij"), true);
    EQ ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false);
    EQ ("abcdefghijklmnopqrst", TypeParam (""), false);
    EQ ("abcdefghijklmnopqrst", TypeParam ("abcde"), false);
    EQ ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false);
    EQ ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true);
#undef EQ
}

TYPED_TEST (SStringViewRelational, Ne) {
#define NE(lhs, rhs, x)                                                                            \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view (lhs);                                    \
        EXPECT_EQ (lhs_view != (rhs), (x));                                                        \
        EXPECT_EQ ((rhs) != lhs_view, (x));                                                        \
    }
    NE ("", TypeParam (""), false);
    NE ("", TypeParam ("abcde"), true);
    NE ("", TypeParam ("abcdefghij"), true);
    NE ("", TypeParam ("abcdefghijklmnopqrst"), true);
    NE ("abcde", TypeParam (""), true);
    NE ("abcde", TypeParam ("abcde"), false);
    NE ("abcde", TypeParam ("abcdefghij"), true);
    NE ("abcde", TypeParam ("abcdefghijklmnopqrst"), true);
    NE ("abcdefghij", TypeParam (""), true);
    NE ("abcdefghij", TypeParam ("abcde"), true);
    NE ("abcdefghij", TypeParam ("abcdefghij"), false);
    NE ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true);
    NE ("abcdefghijklmnopqrst", TypeParam (""), true);
    NE ("abcdefghijklmnopqrst", TypeParam ("abcde"), true);
    NE ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true);
    NE ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false);
}

TYPED_TEST (SStringViewRelational, Ge) {
#define GE(lhs, rhs, x, y)                                                                         \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view (lhs);                                    \
        EXPECT_EQ (lhs_view >= (rhs), (x));                                                        \
        EXPECT_EQ ((rhs) >= lhs_view, (y));                                                        \
    }
    GE ("", TypeParam (""), true, true);
    GE ("", TypeParam ("abcde"), false, true);
    GE ("", TypeParam ("abcdefghij"), false, true);
    GE ("", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GE ("abcde", TypeParam (""), true, false);
    GE ("abcde", TypeParam ("abcde"), true, true);
    GE ("abcde", TypeParam ("abcdefghij"), false, true);
    GE ("abcde", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GE ("abcdefghij", TypeParam (""), true, false);
    GE ("abcdefghij", TypeParam ("abcde"), true, false);
    GE ("abcdefghij", TypeParam ("abcdefghij"), true, true);
    GE ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GE ("abcdefghijklmnopqrst", TypeParam (""), true, false);
    GE ("abcdefghijklmnopqrst", TypeParam ("abcde"), true, false);
    GE ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true, false);
    GE ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true, true);
#undef GE
}

TYPED_TEST (SStringViewRelational, Gt) {
#define GT(lhs, rhs, x, y)                                                                         \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view (lhs);                                    \
        EXPECT_EQ (lhs_view > (rhs), (x));                                                         \
        EXPECT_EQ ((rhs) > lhs_view, (y));                                                         \
    }
    GT ("", TypeParam (""), false, false);
    GT ("", TypeParam ("abcde"), false, true);
    GT ("", TypeParam ("abcdefghij"), false, true);
    GT ("", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GT ("abcde", TypeParam (""), true, false);
    GT ("abcde", TypeParam ("abcde"), false, false);
    GT ("abcde", TypeParam ("abcdefghij"), false, true);
    GT ("abcde", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GT ("abcdefghij", TypeParam (""), true, false);
    GT ("abcdefghij", TypeParam ("abcde"), true, false);
    GT ("abcdefghij", TypeParam ("abcdefghij"), false, false);
    GT ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), false, true);
    GT ("abcdefghijklmnopqrst", TypeParam (""), true, false);
    GT ("abcdefghijklmnopqrst", TypeParam ("abcde"), true, false);
    GT ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), true, false);
    GT ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false, false);
#undef GT
}

TYPED_TEST (SStringViewRelational, Le) {
#define LE(lhs, rhs, x, y)                                                                         \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view ((lhs));                                  \
        EXPECT_EQ (lhs_view <= (rhs), bool{(x)});                                                  \
        EXPECT_EQ ((rhs) <= lhs_view, bool{(y)});                                                  \
    }
    LE ("", TypeParam (""), true, true);
    LE ("", TypeParam ("abcde"), true, false);
    LE ("", TypeParam ("abcdefghij"), true, false);
    LE ("", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LE ("abcde", TypeParam (""), false, true);
    LE ("abcde", TypeParam ("abcde"), true, true);
    LE ("abcde", TypeParam ("abcdefghij"), true, false);
    LE ("abcde", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LE ("abcdefghij", TypeParam (""), false, true);
    LE ("abcdefghij", TypeParam ("abcde"), false, true);
    LE ("abcdefghij", TypeParam ("abcdefghij"), true, true);
    LE ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LE ("abcdefghijklmnopqrst", TypeParam (""), false, true);
    LE ("abcdefghijklmnopqrst", TypeParam ("abcde"), false, true);
    LE ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false, true);
    LE ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), true, true);
#undef LE
}

TYPED_TEST (SStringViewRelational, Lt) {
#define LT(lhs, rhs, x, y)                                                                         \
    {                                                                                              \
        pstore::sstring_view lhs_view = new_sstring_view ((lhs));                                  \
        EXPECT_EQ ((lhs_view < rhs), bool{(x)});                                                   \
        EXPECT_EQ ((rhs < lhs_view), bool{(y)});                                                   \
    }
    LT ("", TypeParam (""), false, false);
    LT ("", TypeParam ("abcde"), true, false);
    LT ("", TypeParam ("abcdefghij"), true, false);
    LT ("", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LT ("abcde", TypeParam (""), false, true);
    LT ("abcde", TypeParam ("abcde"), false, false);
    LT ("abcde", TypeParam ("abcdefghij"), true, false);
    LT ("abcde", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LT ("abcdefghij", TypeParam (""), false, true);
    LT ("abcdefghij", TypeParam ("abcde"), false, true);
    LT ("abcdefghij", TypeParam ("abcdefghij"), false, false);
    LT ("abcdefghij", TypeParam ("abcdefghijklmnopqrst"), true, false);
    LT ("abcdefghijklmnopqrst", TypeParam (""), false, true);
    LT ("abcdefghijklmnopqrst", TypeParam ("abcde"), false, true);
    LT ("abcdefghijklmnopqrst", TypeParam ("abcdefghij"), false, true);
    LT ("abcdefghijklmnopqrst", TypeParam ("abcdefghijklmnopqrst"), false, false);
#undef LE
}
// eof: unittests/pstore/test_sstring_view.cpp
