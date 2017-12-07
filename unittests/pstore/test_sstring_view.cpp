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
#include <cstring>
#include <gmock/gmock.h>
#include "pstore/make_unique.hpp"

namespace {
    class SStringView : public ::testing::Test {
    protected:
        std::shared_ptr<char> new_shared (std::string const & s) {
            auto result = std::shared_ptr<char> (new char[s.size ()], [](char * p) { delete[] p; });
            std::copy (std::begin (s), std::end (s), result.get ());
            return result;
        }
    };
} // anonymous namespace

TEST_F (SStringView, Init) {
    pstore::sstring_view sv;
    EXPECT_EQ (sv.size (), 0U);
    EXPECT_EQ (sv.length (), 0U);
    EXPECT_EQ (sv.max_size (), std::numeric_limits<std::size_t>::max ());
    EXPECT_TRUE (sv.empty ());
    EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 0);
}

TEST_F (SStringView, At) {
    std::string const src{"ABCDE"};
    auto const sv = pstore::sstring_view::make (src);
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
    pstore::sstring_view sv{ptr, length};

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.back (), src[length - 1]);
    EXPECT_EQ (&sv.back (), ptr.get () + length - 1);
}

TEST_F (SStringView, Data) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv{ptr, length};

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.data (), ptr.get ());
}

TEST_F (SStringView, Front) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv{ptr, length};

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.front (), src[0]);
    EXPECT_EQ (&sv.front (), ptr.get ());
}

TEST_F (SStringView, Index) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view sv{ptr, length};

    EXPECT_EQ (sv[0], src[0]);
    EXPECT_EQ (&sv[0], ptr.get () + 0);
    EXPECT_EQ (sv[1], src[1]);
    EXPECT_EQ (&sv[1], ptr.get () + 1);
    EXPECT_EQ (sv[4], src[4]);
    EXPECT_EQ (&sv[4], ptr.get () + 4);
}

TEST_F (SStringView, RBeginEmpty) {
    pstore::sstring_view sv = pstore::sstring_view::make ("");
    pstore::sstring_view const & csv = sv;

    pstore::sstring_view::reverse_iterator rbegin = sv.rbegin ();
    pstore::sstring_view::const_reverse_iterator const_rbegin1 = csv.rbegin ();
    pstore::sstring_view::const_reverse_iterator const_rbegin2 = sv.crbegin ();
    EXPECT_EQ (rbegin, const_rbegin1);
    EXPECT_EQ (rbegin, const_rbegin2);
    EXPECT_EQ (const_rbegin1, const_rbegin2);
}

TEST_F (SStringView, RBegin) {
    pstore::sstring_view sv = pstore::sstring_view::make ("abc");
    pstore::sstring_view const & csv = sv;

    pstore::sstring_view::reverse_iterator rbegin = sv.rbegin ();
    pstore::sstring_view::const_reverse_iterator const_begin1 = csv.rbegin ();
    pstore::sstring_view::const_reverse_iterator const_begin2 = sv.crbegin ();

    std::size_t const last = sv.size () - 1;
    EXPECT_EQ (*rbegin, sv[last]);
    EXPECT_EQ (&*rbegin, &sv[last]);
    EXPECT_EQ (*const_begin1, sv[last]);
    EXPECT_EQ (&*const_begin1, &sv[last]);
    EXPECT_EQ (*const_begin2, sv[last]);
    EXPECT_EQ (&*const_begin2, &sv[last]);

    EXPECT_EQ (rbegin, const_begin1);
    EXPECT_EQ (rbegin, const_begin2);
    EXPECT_EQ (const_begin1, const_begin2);
}

TEST_F (SStringView, REndEmpty) {
    pstore::sstring_view sv = pstore::sstring_view::make ("");
    pstore::sstring_view const & csv = sv;
    pstore::sstring_view::reverse_iterator rend = sv.rend ();
    pstore::sstring_view::const_reverse_iterator const_rend1 = csv.rend ();
    pstore::sstring_view::const_reverse_iterator const_rend2 = sv.crend ();

    EXPECT_EQ (rend, sv.rbegin ());
    EXPECT_EQ (const_rend1, csv.rbegin ());
    EXPECT_EQ (const_rend2, sv.rbegin ());

    EXPECT_EQ (rend - sv.rbegin (), 0);
    EXPECT_EQ (const_rend1 - csv.rbegin (), 0);
    EXPECT_EQ (const_rend2 - sv.crbegin (), 0);

    EXPECT_EQ (rend, const_rend1);
    EXPECT_EQ (rend, const_rend2);
    EXPECT_EQ (const_rend1, const_rend2);
}

TEST_F (SStringView, REnd) {
    pstore::sstring_view sv = pstore::sstring_view::make ("abc");
    pstore::sstring_view const & csv = sv;
    pstore::sstring_view::reverse_iterator rend = sv.rend ();
    pstore::sstring_view::const_reverse_iterator const_rend1 = csv.rend ();
    pstore::sstring_view::const_reverse_iterator const_rend2 = sv.crend ();

    EXPECT_NE (rend, sv.rbegin ());
    EXPECT_NE (const_rend1, csv.rbegin ());
    EXPECT_NE (const_rend2, sv.rbegin ());

    EXPECT_EQ (rend - sv.rbegin (), 3);
    EXPECT_EQ (const_rend1 - csv.rbegin (), 3);
    EXPECT_EQ (const_rend2 - sv.crbegin (), 3);

    EXPECT_EQ (rend, const_rend1);
    EXPECT_EQ (rend, const_rend2);
    EXPECT_EQ (const_rend1, const_rend2);
}

TEST_F (SStringView, Clear) {
    pstore::sstring_view empty = pstore::sstring_view::make ("");
    {
        pstore::sstring_view sv1 = pstore::sstring_view::make ("abc");
        sv1.clear ();
        EXPECT_EQ (sv1.size (), 0U);
        EXPECT_EQ (sv1, empty);
    }
    {
        pstore::sstring_view sv2 = pstore::sstring_view::make ("");
        sv2.clear ();
        EXPECT_EQ (sv2.size (), 0U);
        EXPECT_EQ (sv2, empty);
    }
}

namespace {
    template <typename StringType>
    class SStringViewRelational : public SStringView {};

    class sstringview_maker {
    public:
        sstringview_maker (char const * s)
                : view_ (pstore::sstring_view::make (s)) {}
        operator pstore::sstring_view () const noexcept {
            return view_;
        }
    private:
        pstore::sstring_view view_;
    };

    using StringTypes = ::testing::Types<sstringview_maker, sstringview_maker const, char const *,
                                         std::string, std::string const>;
} // anonymous namespace

namespace pstore {
    template <>
    struct string_traits<sstringview_maker> {
        static std::size_t length (sstring_view const & s) noexcept {
            return string_traits<sstring_view>::length (s);
        }
        static char const * data (sstring_view const & s) noexcept {
            return string_traits<sstring_view>::data (s);
        }
    };
} // anonymous namespace

TYPED_TEST_CASE (SStringViewRelational, StringTypes);

TYPED_TEST (SStringViewRelational, Eq) {
#define EQ(lhs, rhs, x)                                                                            \
    {                                                                                              \
        pstore::sstring_view lhs_view = pstore::sstring_view::make (lhs);                          \
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
        pstore::sstring_view lhs_view = pstore::sstring_view::make (lhs);                          \
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
        pstore::sstring_view lhs_view = pstore::sstring_view::make (lhs);                          \
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
        pstore::sstring_view lhs_view = pstore::sstring_view::make (lhs);                          \
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
        pstore::sstring_view lhs_view = pstore::sstring_view::make ((lhs));                        \
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
        pstore::sstring_view lhs_view = pstore::sstring_view::make ((lhs));                        \
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

namespace {


    void test (char const * str, pstore::sstring_view::value_type c,
               pstore::sstring_view::size_type x) {
        EXPECT_EQ (pstore::sstring_view::make (str).find (c), x);
        if (x != pstore::sstring_view::npos) {
            EXPECT_GE (x, 0U);
            EXPECT_LE (x + 1, std::strlen (str));
        }
    }
}

TEST_F (SStringView, FindCharAndPos) {
#define CHECK(str, c, pos, expected)                                                               \
    assert ((expected) == pstore::sstring_view::npos || (expected) >= (pos));                      \
    assert ((expected) == pstore::sstring_view::npos || (expected) < std::strlen (str));           \
    EXPECT_EQ (pstore::sstring_view::make (str).find (c, pos), expected);


    CHECK ("", 'c', 0, pstore::sstring_view::npos);
    CHECK ("", 'c', 1, pstore::sstring_view::npos);
    CHECK ("abcde", 'c', 0, 2U);
    CHECK ("abcde", 'c', 1, 2U);
    CHECK ("abcde", 'c', 2, 2U);
    CHECK ("abcde", 'c', 4, pstore::sstring_view::npos);
    CHECK ("abcde", 'c', 5, pstore::sstring_view::npos);
    CHECK ("abcde", 'c', 6, pstore::sstring_view::npos);
    CHECK ("abcdeabcde", 'c', 0, 2U);
    CHECK ("abcdeabcde", 'c', 1, 2U);
    CHECK ("abcdeabcde", 'c', 5, 7U);
    CHECK ("abcdeabcde", 'c', 9, pstore::sstring_view::npos);
    CHECK ("abcdeabcde", 'c', 10, pstore::sstring_view::npos);
    CHECK ("abcdeabcde", 'c', 11, pstore::sstring_view::npos);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 0, 2U);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 1, 2U);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 10, 12U);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 19, pstore::sstring_view::npos);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 20, pstore::sstring_view::npos);
    CHECK ("abcdeabcdeabcdeabcde", 'c', 21, pstore::sstring_view::npos);
#undef CHECK
}
TEST_F (SStringView, FindChar) {
    test ("", 'c', pstore::sstring_view::npos);
    test ("abcde", 'c', 2);
    test ("abcdeabcde", 'c', 2);
    test ("abcdeabcdeabcdeabcde", 'c', 2);
}

TEST_F (SStringView, OperatorWrite) {
    std::ostringstream str;

    test ("", 'c', pstore::sstring_view::npos);
    test ("abcde", 'c', 2);
    test ("abcdeabcde", 'c', 2);
    test ("abcdeabcdeabcdeabcde", 'c', 2);
}
// eof: unittests/pstore/test_sstring_view.cpp
