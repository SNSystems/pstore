//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//===- unittests/pstore_support/test_sstring_view.cpp ---------------------===//
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

#include "pstore_support/sstring_view.hpp"
#include <cstring>
#include <gmock/gmock.h>
#include "pstore/make_unique.hpp"

namespace {
    std::shared_ptr<char> new_shared (std::string const & s) {
        auto result = std::shared_ptr<char> (new char[s.size ()], [](char * p) { delete[] p; });
        std::copy (std::begin (s), std::end (s), result.get ());
        return result;
    }

    class SStringView : public ::testing::Test {
    protected:
    };
} // anonymous namespace

namespace {
    template <typename T>
    struct string_maker {};

    template <typename CharType>
    struct string_maker<std::shared_ptr<CharType>> {
        std::shared_ptr<CharType> operator() (std::string const & str) const {
            auto result =
                std::shared_ptr<char> (new char[str.length ()], [](char * p) { delete[] p; });
            auto ptr = const_cast<typename std::remove_const<CharType>::type *> (result.get ());
            std::copy (std::begin (str), std::end (str), ptr);
            return result;
        }
    };

    template <typename CharType>
    struct string_maker<std::unique_ptr<CharType[]>> {
        std::unique_ptr<CharType[]> operator() (std::string const & str) const {
            auto result = std::make_unique<CharType[]> (str.length ());
            auto ptr = const_cast<typename std::remove_const<CharType>::type *> (result.get ());
            std::copy (std::begin (str), std::end (str), ptr);
            return result;
        }
    };
    template <>
    struct string_maker<char const *> {
        char const * operator() (std::string const & str) const { return str.data (); }
    };

    template <typename StringType>
    class SStringViewInit : public ::testing::Test {};
    using SStringViewInitTypes =
        ::testing::Types<string_maker<std::shared_ptr<char>>,
                         string_maker<std::shared_ptr<char const>>,
                         string_maker<std::unique_ptr<char[]>>,
                         string_maker<std::unique_ptr<char const[]>>, string_maker<char const *>>;
} // anonymous namespace

TYPED_TEST_CASE (SStringViewInit, SStringViewInitTypes);

TYPED_TEST (SStringViewInit, Empty) {
    TypeParam t;
    std::string const src{""};
    auto ptr = t (src);
    auto sv = pstore::make_sstring_view (std::move (ptr), src.length ());
    EXPECT_EQ (sv.size (), 0U);
    EXPECT_EQ (sv.length (), 0U);
    EXPECT_EQ (sv.max_size (), std::numeric_limits<std::size_t>::max ());
    EXPECT_TRUE (sv.empty ());
    EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 0);
}

TYPED_TEST (SStringViewInit, Short) {
    TypeParam t;
    std::string const src{"hello"};
    auto ptr = t (src);
    auto sv = pstore::make_sstring_view (std::move (ptr), src.length ());
    EXPECT_EQ (sv.size (), 5U);
    EXPECT_EQ (sv.length (), 5U);
    EXPECT_EQ (sv.max_size (), std::numeric_limits<std::size_t>::max ());
    EXPECT_FALSE (sv.empty ());
    EXPECT_EQ (std::distance (std::begin (sv), std::end (sv)), 5);
}


TEST_F (SStringView, OperatorIndex) {
    std::string const src{"ABCDE"};
    pstore::sstring_view<char const *> sv = pstore::make_sstring_view (src.data (), src.length ());
    ASSERT_EQ (sv.length (), src.length ());
    EXPECT_FALSE (sv.empty ());
    EXPECT_EQ (sv[0], 'A');
    EXPECT_EQ (sv[1], 'B');
    EXPECT_EQ (sv[4], 'E');
}

TEST_F (SStringView, At) {
    std::string const src{"ABCDE"};
    pstore::sstring_view<char const *> sv = pstore::make_sstring_view (src.data (), src.length ());
    ASSERT_EQ (sv.length (), src.length ());
    EXPECT_FALSE (sv.empty ());
    EXPECT_EQ (sv.at (0), 'A');
    EXPECT_EQ (sv.at (1), 'B');
    EXPECT_EQ (sv.at (4), 'E');
#if PSTORE_CPP_EXCEPTIONS
    EXPECT_THROW (sv.at (5), std::out_of_range);
#endif
}

TEST_F (SStringView, Back) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_sstring_view (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.back (), src[length - 1]);
    EXPECT_EQ (&sv.back (), sv.data () + length - 1U);
}

TEST_F (SStringView, Data) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_sstring_view (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.data (), ptr.get ());
}

TEST_F (SStringView, Front) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_sstring_view (ptr, length);

    ASSERT_EQ (sv.length (), length);
    EXPECT_EQ (sv.front (), src[0]);
    EXPECT_EQ (&sv.front (), sv.data ());
}

TEST_F (SStringView, Index) {
    std::string const src{"ABCDE"};
    auto const length = src.length ();
    std::shared_ptr<char> ptr = new_shared (src);
    pstore::sstring_view<std::shared_ptr<char>> sv = pstore::make_sstring_view (ptr, length);

    EXPECT_EQ (sv[0], src[0]);
    EXPECT_EQ (&sv[0], ptr.get () + 0);
    EXPECT_EQ (sv[1], src[1]);
    EXPECT_EQ (&sv[1], ptr.get () + 1);
    EXPECT_EQ (sv[4], src[4]);
    EXPECT_EQ (&sv[4], ptr.get () + 4);
}

TEST_F (SStringView, RBeginEmpty) {
    std::string const src;
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());
    sv_type const & csv = sv;

    sv_type::reverse_iterator rbegin = sv.rbegin ();
    sv_type::const_reverse_iterator const_rbegin1 = csv.rbegin ();
    sv_type::const_reverse_iterator const_rbegin2 = sv.crbegin ();
    EXPECT_EQ (rbegin, const_rbegin1);
    EXPECT_EQ (rbegin, const_rbegin2);
    EXPECT_EQ (const_rbegin1, const_rbegin2);
}

TEST_F (SStringView, RBegin) {
    std::string const src{"abc"};
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());
    sv_type const & csv = sv;

    sv_type::reverse_iterator rbegin = sv.rbegin ();
    sv_type::const_reverse_iterator const_begin1 = csv.rbegin ();
    sv_type::const_reverse_iterator const_begin2 = sv.crbegin ();

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
    std::string const src{""};
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());
    sv_type const & csv = sv;

    sv_type::reverse_iterator rend = sv.rend ();
    sv_type::const_reverse_iterator const_rend1 = csv.rend ();
    sv_type::const_reverse_iterator const_rend2 = sv.crend ();

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
    std::string const src{"abc"};
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());
    sv_type const & csv = sv;

    sv_type::reverse_iterator rend = sv.rend ();
    sv_type::const_reverse_iterator const_rend1 = csv.rend ();
    sv_type::const_reverse_iterator const_rend2 = sv.crend ();

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
    std::string const empty_str;

    pstore::sstring_view<char const *> empty =
        pstore::make_sstring_view (empty_str.data (), empty_str.length ());
    {
        std::string const abc_str{"abc"};
        pstore::sstring_view<char const *> sv1 =
            pstore::make_sstring_view (abc_str.data (), abc_str.length ());
        sv1.clear ();
        EXPECT_EQ (sv1.size (), 0U);
        EXPECT_EQ (sv1, empty);
    }
    {
        pstore::sstring_view<char const *> sv2 =
            pstore::make_sstring_view (empty_str.data (), empty_str.length ());
        sv2.clear ();
        EXPECT_EQ (sv2.size (), 0U);
        EXPECT_EQ (sv2, empty);
    }
}

TEST_F (SStringView, FindChar) {
    std::string const src{"abc"};
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());

    EXPECT_EQ (sv.find ('a'), 0U);
    EXPECT_EQ (sv.find ('c'), 2U);
    EXPECT_EQ (sv.find ('d'), sv_type::npos);
    EXPECT_EQ (sv.find ('c', 1U), 2U);
    EXPECT_EQ (sv.find ('c', 3U), sv_type::npos);
}

TEST_F (SStringView, Substr) {
    std::string const src{"abc"};
    using sv_type = pstore::sstring_view<char const *>;
    sv_type sv = pstore::make_sstring_view (src.data (), src.length ());

    EXPECT_EQ (sv.substr (0U, 1U), "a");
    EXPECT_EQ (sv.substr (0U, 4U), "abc");
    EXPECT_EQ (sv.substr (1U, 1U), "b");
    EXPECT_EQ (sv.substr (3U, 1U), "");
}

namespace {
    template <typename StringType>
    class SStringViewRelational : public SStringView {};

    class sstringview_maker {
    public:
        sstringview_maker (char const * s)
                : view_ (pstore::make_sstring_view (s, std::strlen (s))) {}
        operator pstore::sstring_view<char const *> () const noexcept { return view_; }

    private:
        pstore::sstring_view<char const *> view_;
    };

    using StringTypes = ::testing::Types<sstringview_maker, sstringview_maker const, char const *>;
} // anonymous namespace

namespace pstore {
    template <>
    struct string_traits<sstringview_maker> {
        static std::size_t length (sstring_view<char const *> const & s) noexcept {
            return string_traits<sstring_view<char const *>>::length (s);
        }
        static char const * data (sstring_view<char const *> const & s) noexcept {
            return string_traits<sstring_view<char const *>>::data (s);
        }
    };
} // namespace pstore

TYPED_TEST_CASE (SStringViewRelational, StringTypes);

TYPED_TEST (SStringViewRelational, Eq) {
#define EQ(lhs, rhs, x)                                                                            \
    {                                                                                              \
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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
        auto lhs_view = pstore::make_sstring_view (lhs, std::strlen (lhs));                        \
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

TEST_F (SStringView, OperatorWrite) {
    auto check = [](std::string const & str) {
        auto view = pstore::make_sstring_view (str.data (), str.length ());
        std::ostringstream stream;
        stream << view;
        EXPECT_EQ (stream.str (), str);
    };
    check ("");
    check ("abcdef");
    check ("hello world");
}
// eof: unittests/pstore_support/test_sstring_view.cpp
