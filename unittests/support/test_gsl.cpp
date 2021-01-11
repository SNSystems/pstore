//*            _  *
//*   __ _ ___| | *
//*  / _` / __| | *
//* | (_| \__ \ | *
//*  \__, |___/_| *
//*  |___/        *
//===- unittests/support/test_gsl.cpp -------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include "pstore/support/gsl.hpp"
#include "gtest/gtest.h"

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <regex>

using namespace pstore::gsl;

namespace {

    struct BaseClass {};
    struct DerivedClass : BaseClass {};

} // end anonymous namespace

TEST (GslSpan, DefaultCtor) {
    {
        span<int> s;
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<int const> cs;
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
        span<int, 0> s;
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<int const, 0> cs;
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
#ifdef CONFIRM_COMPILATION_ERRORS
        span<int, 1> s;
        EXPECT_EQ (s.length () == 1 && s.data (), nullptr); // explains why it can't compile
#endif
    }
    {
        span<int> s{};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<int const> cs{};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
}

TEST (GslSpan, SizeOptimization) {
    {
        span<int> s;
        EXPECT_EQ (sizeof (s), sizeof (int *) + sizeof (ptrdiff_t));
    }
    {
        span<int, 0> s;
        EXPECT_EQ (sizeof (s), sizeof (int *));
    }
}

TEST (GslSpan, FromNullptrCtor) {
    {
        auto s = span<int>{nullptr};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        auto cs = span<int const>{nullptr};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
        auto s = span<int, 0>{nullptr};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        auto cs = span<int const, 0>{nullptr};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }

    {
#ifdef CONFIRM_COMPILATION_ERRORS
        span<int, 1> s = nullptr;
        EXPECT_EQ (s.length () == 1 && s.data (), nullptr); // explains why it can't compile
#endif
    }
    {
        span<int> s{nullptr};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<int const> cs{nullptr};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
        span<int *> s{nullptr};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<int const *> cs{nullptr};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
}

TEST (GslSpan, FromNullptrLengthConstructor) {
    {
        span<int> s{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<const int> cs{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
        span<int, 0> s{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<const int, 0> cs{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
    {
        span<int *> s{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);

        span<const int *> cs{nullptr, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (cs.length (), 0);
        EXPECT_EQ (cs.data (), nullptr);
    }
}

TEST (GslSpan, FromPointerLengthConstructor) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s{&arr[0], 2};
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        span<int, 2> s{&arr[0], 2};
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        int * p = nullptr;
        span<int> s{p, static_cast<span<int>::index_type> (0)};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);
    }
    {
        auto s = make_span (&arr[0], 2);
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        int * p = nullptr;
        auto s = make_span (p, static_cast<span<int>::index_type> (0));
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);
    }
}

TEST (GslSpan, FromPointerPointerConstructor) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s{&arr[0], &arr[2]};
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        span<int, 2> s{&arr[0], &arr[2]};
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        span<int> s{&arr[0], &arr[0]};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), &arr[0]);
    }
    {
        span<int, 0> s{&arr[0], &arr[0]};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), &arr[0]);
    }

    {
        int * p = nullptr;
        span<int> s{p, p};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);
    }
    {
        int * p = nullptr;
        span<int, 0> s{p, p};
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);
    }

    {
        auto s = make_span (&arr[0], &arr[2]);
        ASSERT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[0]);
        EXPECT_EQ (s[0], 1);
        EXPECT_EQ (s[1], 2);
    }
    {
        auto s = make_span (&arr[0], &arr[0]);
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), &arr[0]);
    }

    {
        int * p = nullptr;
        auto s = make_span (p, p);
        EXPECT_EQ (s.length (), 0);
        EXPECT_EQ (s.data (), nullptr);
    }
}


TEST (GslSpan, FromArrayConstructor) {
    int arr[5] = {1, 2, 3, 4, 5};

    {
        span<int> s{arr};
        EXPECT_EQ (s.length (), 5);
        EXPECT_EQ (s.data (), &arr[0]);
    }
    {
        span<int, 5> s{arr};
        EXPECT_EQ (s.length (), 5);
        EXPECT_EQ (s.data (), &arr[0]);
    }

    int arr2d[2][3] = {{1, 2, 3}, {4, 5, 6}};

#ifdef CONFIRM_COMPILATION_ERRORS
    { span<int, 6> s{arr}; }
    {
        span<int, 0> s{arr};
        EXPECT_EQ (s.length () == 0 && s.data (), &arr[0]);
    }
    {
        span<int> s{arr2d};
        EXPECT_EQ (s.length () == 6 && s.data (), &arr2d[0][0]);
        EXPECT_EQ (s[0] == 1 && s[5], 6);
    }
    {
        span<int, 0> s{arr2d};
        EXPECT_EQ (s.length () == 0 && s.data (), &arr2d[0][0]);
    }
    { span<int, 6> s{arr2d}; }
#endif
    {
        span<int[3]> s{&(arr2d[0]), 1};
        EXPECT_EQ (s.length (), 1);
        EXPECT_EQ (s.data (), &arr2d[0]);
    }

    int arr3d[2][3][2] = {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}}};

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        span<int> s{arr3d};
        EXPECT_EQ (s.length () == 12 && s.data (), &arr3d[0][0][0]);
        EXPECT_EQ (s[0] == 1 && s[11], 12);
    }
    {
        span<int, 0> s{arr3d};
        EXPECT_EQ (s.length () == 0 && s.data (), &arr3d[0][0][0]);
    }
    { span<int, 11> s{arr3d}; }
    {
        span<int, 12> s{arr3d};
        EXPECT_EQ (s.length () == 12 && s.data (), &arr3d[0][0][0]);
        EXPECT_EQ (s[0] == 1 && s[5], 6);
    }
#endif
    {
        span<int[3][2]> s{&arr3d[0], 1};
        EXPECT_EQ (s.length (), 1);
        EXPECT_EQ (s.data (), &arr3d[0]);
    }
    {
        auto s = make_span (arr);
        EXPECT_EQ (decltype (s)::extent, 5);
        EXPECT_EQ (s.length (), 5);
        EXPECT_EQ (s.data (), &arr[0]);
    }
    {
        auto s = make_span (&(arr2d[0]), 1);
        EXPECT_EQ (decltype (s)::extent, dynamic_extent);
        EXPECT_EQ (s.length (), 1);
        EXPECT_EQ (s.data (), &arr2d[0]);
    }
    {
        auto s = make_span (&arr3d[0], 1);
        EXPECT_EQ (decltype (s)::extent, dynamic_extent);
        EXPECT_EQ (s.length (), 1);
        EXPECT_EQ (s.data (), &arr3d[0]);
    }
}

TEST (GslSpan, FromDynamicArrayConstructor) {
    double (*arr)[3][4] = new double[100][3][4];

    {
        span<double> s (&arr[0][0][0], 10);
        EXPECT_EQ (s.length (), 10);
        EXPECT_EQ (s.data (), &arr[0][0][0]);
    }
    {
        auto s = make_span (&arr[0][0][0], 10);
        EXPECT_EQ (s.length (), 10);
        EXPECT_EQ (s.data (), &arr[0][0][0]);
    }

    delete[] arr;
}

TEST (GslSpan, FromStdArrayConstructor) {
    std::array<int, 4> arr = {{1, 2, 3, 4}};

    {
        span<int> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());

        span<int, 4> cs{arr};
        EXPECT_EQ (cs.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (cs.data (), arr.data ());
    }
    {
        span<int, 4> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());

        span<int const, 4> cs{arr};
        EXPECT_EQ (cs.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (cs.data (), arr.data ());
    }

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        span<int, 2> s{arr};
        EXPECT_EQ (s.size () == 2 && s.data (), arr.data ());

        span<const int, 2> cs{arr};
        EXPECT_EQ (cs.size () == 2 && cs.data (), arr.data ());
    }
    {
        span<int, 0> s{arr};
        EXPECT_EQ (s.size () == 0 && s.data (), arr.data ());

        span<const int, 0> cs{arr};
        EXPECT_EQ (cs.size () == 0 && cs.data (), arr.data ());
    }
    { span<int, 5> s{arr}; }
    {
        auto get_an_array = [] () -> std::array<int, 4> { return {1, 2, 3, 4}; };
        auto take_a_span = [] (span<int> s) { static_cast<void> (s); };
        // try to take a temporary std::array
        take_a_span (get_an_array ());
    }
#endif

    {
        auto get_an_array = [] () -> std::array<int, 4> { return {{1, 2, 3, 4}}; };
        auto take_a_span = [] (span<int const> s) { static_cast<void> (s); };
        // try to take a temporary std::array
        take_a_span (span<int const, 4>{get_an_array ()});
    }
    {
        auto s = make_span (arr);
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }
}

TEST (GslSpan, FromConstStdArrayConstructor) {
    std::array<int, 4> const arr = {{1, 2, 3, 4}};

    {
        span<const int> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }
    {
        span<const int, 4> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        span<const int, 2> s{arr};
        EXPECT_EQ (s.size () == 2 && s.data (), arr.data ());
    }
    {
        span<const int, 0> s{arr};
        EXPECT_EQ (s.size () == 0 && s.data (), arr.data ());
    }
    { span<const int, 5> s{arr}; }
#endif

    {
        auto get_an_array = [] () -> const std::array<int, 4> { return {{1, 2, 3, 4}}; };
        auto take_a_span = [] (span<int const> s) { static_cast<void> (s); };
        // try to take a temporary std::array
        take_a_span (span<int const, 4>{get_an_array ()});
    }
    {
        auto s = make_span (arr);
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }
}

TEST (GslSpan, FromStdArrayConstConstructor) {
    std::array<const int, 4> arr = {{1, 2, 3, 4}};

    {
        span<const int> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }
    {
        span<const int, 4> s{arr};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }

#ifdef CONFIRM_COMPILATION_ERRORS
    {
        span<const int, 2> s{arr};
        EXPECT_EQ (s.size () == 2 && s.data (), arr.data ());
    }
    {
        span<const int, 0> s{arr};
        EXPECT_EQ (s.size () == 0 && s.data (), arr.data ());
    }
    { span<const int, 5> s{arr}; }
    { span<int, 4> s{arr}; }
#endif

    {
        auto s = make_span (arr);
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (arr.size ()));
        EXPECT_EQ (s.data (), arr.data ());
    }
}

TEST (GslSpan, FromUniquePointerConstruction) {
    {
        auto ptr = std::make_unique<int> (4);
        {
            span<int> s{ptr};
            EXPECT_EQ (s.length (), 1);
            EXPECT_EQ (s.data (), ptr.get ());
            EXPECT_EQ (s[0], 4);
        }
        {
            auto s = make_span (ptr);
            EXPECT_EQ (s.length (), 1);
            EXPECT_EQ (s.data (), ptr.get ());
            EXPECT_EQ (s[0], 4);
        }
    }
    {
        auto ptr = std::unique_ptr<int>{nullptr};
        {
            span<int> s{ptr};
            EXPECT_EQ (s.length (), 0);
            EXPECT_EQ (s.data (), nullptr);
        }
        {
            auto s = make_span (ptr);
            EXPECT_EQ (s.length (), 0);
            EXPECT_EQ (s.data (), nullptr);
        }
    }
    {
        auto arr = std::make_unique<int[]> (4U);
        for (auto i = 0U; i < 4U; i++) {
            arr[i] = static_cast<int> (i) + 1;
        }

        {
            span<int> s{arr, 4};
            EXPECT_EQ (s.length (), 4);
            EXPECT_EQ (s.data (), arr.get ());
            EXPECT_EQ (s[0], 1);
            EXPECT_EQ (s[1], 2);
        }
        {
            auto s = make_span (arr, 4);
            EXPECT_EQ (s.length (), 4);
            EXPECT_EQ (s.data (), arr.get ());
            EXPECT_EQ (s[0], 1);
            EXPECT_EQ (s[1], 2);
        }
    }
    {
        auto arr = std::unique_ptr<int[]>{nullptr};
        {
            span<int> s{arr, 0};
            EXPECT_EQ (s.length (), 0);
            EXPECT_EQ (s.data (), nullptr);
        }
        {
            auto s = make_span (arr, 0);
            EXPECT_EQ (s.length (), 0);
            EXPECT_EQ (s.data (), nullptr);
        }
    }
}

#if 0
TEST (GslSpan, FromSharedPointerConstruction) {
    {
        auto ptr = std::make_shared<int>(4);
        {
            span<int> s{ptr};
            EXPECT_EQ (s.length(), 1);
            EXPECT_EQ (s.data(), ptr.get());
            EXPECT_EQ (s[0], 4);
        }
        {
            auto s = make_span(ptr);
            EXPECT_EQ (s.length(), 1);
            EXPECT_EQ (s.data(), ptr.get());
            EXPECT_EQ (s[0], 4);
        }
    }
    {
        auto ptr = std::shared_ptr<int>{nullptr};
        {
            span<int> s{ptr};
            EXPECT_EQ (s.length(), 0);
            EXPECT_EQ (s.data(), nullptr);
        }
        {
            auto s = make_span(ptr);
            EXPECT_EQ (s.length(), 0);
            EXPECT_EQ (s.data(), nullptr);
        }
    }
}
#endif

TEST (GslSpan, FromContainerConstructor) {
    std::vector<int> v = {1, 2, 3};
    std::vector<int> const cv = v;

    {
        span<int> s{v};
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (v.size ()));
        EXPECT_EQ (s.data (), v.data ());

        span<const int> cs{v};
        EXPECT_EQ (cs.size (), static_cast<std::ptrdiff_t> (v.size ()));
        EXPECT_EQ (cs.data (), v.data ());
    }

#if 0
    std::string str = "hello";
    std::string const cstr = "hello";

    {
#    ifdef CONFIRM_COMPILATION_ERRORS
        span<char> s{str};
        EXPECT_EQ (s.size() == narrow_cast<std::ptrdiff_t>(str.size()) && s.data(), str.data());
#    endif
        span<char const > cs{str};
        EXPECT_EQ (cs.size(), static_cast<std::ptrdiff_t>(str.size()));
        EXPECT_EQ (cs.data(), str.data());
    }
    {
#    ifdef CONFIRM_COMPILATION_ERRORS
        span<char> s{cstr};
#    endif
        span<const char> cs{cstr};
        EXPECT_EQ (cs.size(), static_cast<std::ptrdiff_t>(cstr.size()));
        EXPECT_EQ (cs.data(), cstr.data());
    }
#endif
    {
#ifdef CONFIRM_COMPILATION_ERRORS
        auto get_temp_vector = [] () -> std::vector<int> { return {}; };
        auto use_span = [] (span<int> s) { static_cast<void> (s); };
        use_span (get_temp_vector ());
#endif
    }
    {
        auto get_temp_vector = [] () -> std::vector<int> { return {}; };
        auto use_span = [] (span<int const> s) { static_cast<void> (s); };
        use_span (span<int const> (get_temp_vector ()));
    }
    {
#ifdef CONFIRM_COMPILATION_ERRORS
        auto get_temp_string = [] () -> std::string { return {}; };
        auto use_span = [] (span<char> s) { static_cast<void> (s); };
        use_span (get_temp_string ());
#endif
    }
#if 0
    {
        auto get_temp_string = []() -> std::string { return {}; };
        auto use_span = [](span<char const> s) { static_cast<void>(s); };
        use_span(get_temp_string());
    }
    {
#    ifdef CONFIRM_COMPILATION_ERRORS
        auto get_temp_vector = []() -> const std::vector<int> { return {}; };
        auto use_span = [](span<const char> s) { static_cast<void>(s); };
        use_span(get_temp_vector());
#    endif
    }
    {
        auto get_temp_string = []() -> std::string const { return {}; };
        auto use_span = [](span<char const> s) { static_cast<void>(s); };
        use_span(get_temp_string());
    }
#endif
    {
#ifdef CONFIRM_COMPILATION_ERRORS
        std::map<int, int> m;
        span<int> s{m};
#endif
    }
    {
        auto s = make_span (v);
        EXPECT_EQ (s.size (), static_cast<std::ptrdiff_t> (v.size ()));
        EXPECT_EQ (s.data (), v.data ());

        auto cs = make_span (cv);
        EXPECT_EQ (cs.size (), static_cast<std::ptrdiff_t> (cv.size ()));
        EXPECT_EQ (cs.data (), cv.data ());
    }
}

TEST (GslSpan, FromConvertibleSpanConstructor){{span<DerivedClass> avd;
span<DerivedClass const> avcd = avd;
static_cast<void> (avcd);
}
{
#ifdef CONFIRM_COMPILATION_ERRORS
    span<DerivedClass> avd;
    span<BaseClass> avb = avd;
    static_cast<void> (avb);
#endif
}

#ifdef CONFIRM_COMPILATION_ERRORS
{
    span<int> s;
    span<unsigned int> s2 = s;
    static_cast<void> (s2);
}
{
    span<int> s;
    span<const unsigned int> s2 = s;
    static_cast<void> (s2);
}
{
    span<int> s;
    span<short> s2 = s;
    static_cast<void> (s2);
}
#endif
}

TEST (GslSpan, CopyMoveAndAssignment) {
    span<int> s1;
    EXPECT_TRUE (s1.empty ());

    int arr[] = {3, 4, 5};

    auto s2 = span<int const>{arr};
    EXPECT_EQ (s2.length (), 3);
    EXPECT_EQ (s2.data (), &arr[0]);

    s2 = s1;
    EXPECT_TRUE (s2.empty ());

    auto get_temp_span = [&] () -> span<int> { return {&arr[1], 2}; };
    auto use_span = [&] (span<int const> s) {
        EXPECT_EQ (s.length (), 2);
        EXPECT_EQ (s.data (), &arr[1]);
    };
    use_span (get_temp_span ());

    s1 = get_temp_span ();
    EXPECT_EQ (s1.length (), 2);
    EXPECT_EQ (s1.data (), &arr[1]);
}

TEST (GslSpan, First) {
    int arr[5] = {1, 2, 3, 4, 5};

    {
        auto av = span<int, 5>{arr};
        EXPECT_EQ (av.first<2> ().length (), 2);
        EXPECT_EQ (av.first (2).length (), 2);
    }
    {
        auto av = span<int, 5>{arr};
        EXPECT_EQ (av.first<0> ().length (), 0);
        EXPECT_EQ (av.first (0).length (), 0);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ (av.first<5> ().length (), 5);
        EXPECT_EQ (av.first (5).length (), 5);
    }
    {
        span<int> av;
        EXPECT_EQ (av.first<0> ().length (), 0);
        EXPECT_EQ (av.first (0).length (), 0);
    }
}

TEST (GslSpan, Last) {
    int arr[5] = {1, 2, 3, 4, 5};

    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ (av.last<2> ().length (), 2);
        EXPECT_EQ (av.last (2).length (), 2);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ (av.last<0> ().length (), 0);
        EXPECT_EQ (av.last (0).length (), 0);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ (av.last<5> ().length (), 5);
        EXPECT_EQ (av.last (5).length (), 5);
    }
    {
        span<int> av;
        EXPECT_EQ (av.last<0> ().length (), 0);
        EXPECT_EQ (av.last (0).length (), 0);
    }
}

TEST (GslSpan, Subspan) {
    int arr[5] = {1, 2, 3, 4, 5};

    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ ((av.subspan<2, 2> ().length ()), 2);
        EXPECT_EQ (av.subspan (2, 2).length (), 2);
        EXPECT_EQ (av.subspan (2, 3).length (), 3);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ ((av.subspan<0, 0> ().length ()), 0);
        EXPECT_EQ (av.subspan (0, 0).length (), 0);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ ((av.subspan<0, 5> ().length ()), 5);
        EXPECT_EQ ((av.subspan (0, 5).length ()), 5);
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ ((av.subspan<4, 0> ().length ()), 0);
        EXPECT_EQ (av.subspan (4, 0).length (), 0);
        EXPECT_EQ (av.subspan (5, 0).length (), 0);
    }
    {
        span<int> av;
        EXPECT_EQ ((av.subspan<0, 0> ().length ()), 0);
        EXPECT_EQ (av.subspan (0, 0).length (), 0);
    }
    {
        span<int> av;
        EXPECT_EQ (av.subspan (0).length (), 0);
    }
    {
        span<int> av = make_span (arr);
        EXPECT_EQ (av.subspan (0).length (), 5);
        EXPECT_EQ (av.subspan (1).length (), 4);
        EXPECT_EQ (av.subspan (4).length (), 1);
        EXPECT_EQ (av.subspan (5).length (), 0);
        auto av2 = av.subspan (1);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ (av2[i], i + 2);
        }
    }
    {
        span<int, 5> av = make_span (arr);
        EXPECT_EQ (av.subspan (0).length (), 5);
        EXPECT_EQ (av.subspan (1).length (), 4);
        EXPECT_EQ (av.subspan (4).length (), 1);
        EXPECT_EQ (av.subspan (5).length (), 0);
        auto av2 = av.subspan (1);
        for (int i = 0; i < 4; ++i) {
            EXPECT_EQ (av2[i], i + 2);
        }
    }
}

TEST (GslSpan, At) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s = make_span (arr);
        EXPECT_EQ (s.at (0), 1);
    }
    {
        int arr2d[2] = {1, 6};
        span<int, 2> s = make_span (arr2d);
        EXPECT_EQ (s.at (0), 1);
        EXPECT_EQ (s.at (1), 6);
    }
}

TEST (GslSpan, OperatorFunctionCall) {
    int arr[4] = {1, 2, 3, 4};

    {
        span<int> s = make_span (arr);
        EXPECT_EQ (s (0), 1);
    }
    {
        int arr2d[2] = {1, 6};
        span<int, 2> s = make_span (arr2d);
        EXPECT_EQ (s (0), 1);
        EXPECT_EQ (s (1), 6);
    }
}

TEST (GslSpan, IteratorDefaultInit) {
    span<int>::iterator it1;
    span<int>::iterator it2;
    EXPECT_EQ (it1, it2);
}

TEST (GslSpan, ConstIteratorDefaultInit) {
    span<int>::const_iterator it1;
    span<int>::const_iterator it2;
    EXPECT_EQ (it1, it2);
}

TEST (GslSpan, IteratorConversions) {
    span<int>::iterator badIt;
    span<int>::const_iterator badConstIt;
    EXPECT_EQ (badIt, badConstIt);

    int a[] = {1, 2, 3, 4};
    span<int> s = make_span (a);

    auto it = s.begin ();
    auto cit = s.cbegin ();

    EXPECT_EQ (it, cit);
    EXPECT_EQ (cit, it);

    span<int>::const_iterator cit2 = it;
    EXPECT_EQ (cit2, cit);

    span<int>::const_iterator cit3 = it + 4;
    EXPECT_EQ (cit3, s.cend ());
}

TEST (GslSpan, IteratorComparisons) {
    int a[] = {1, 2, 3, 4};
    {
        span<int> s = make_span (a);
        span<int>::iterator it = s.begin ();
        auto it2 = it + 1;
        span<int>::const_iterator cit = s.cbegin ();

        EXPECT_EQ (it, cit);
        EXPECT_EQ (cit, it);
        EXPECT_EQ (it, it);
        EXPECT_EQ (cit, cit);
        EXPECT_EQ (cit, s.begin ());
        EXPECT_EQ (s.begin (), cit);
        EXPECT_EQ (s.cbegin (), cit);
        EXPECT_EQ (it, s.begin ());
        EXPECT_EQ (s.begin (), it);

        EXPECT_TRUE (it != it2);
        EXPECT_TRUE (it2 != it);
        EXPECT_TRUE (it != s.end ());
        EXPECT_TRUE (it2 != s.end ());
        EXPECT_TRUE (s.end () != it);
        EXPECT_TRUE (it2 != cit);
        EXPECT_TRUE (cit != it2);

        EXPECT_TRUE (it < it2);
        EXPECT_TRUE (it <= it2);
        EXPECT_TRUE (it2 <= s.end ());
        EXPECT_TRUE (it < s.end ());
        EXPECT_TRUE (it <= cit);
        EXPECT_TRUE (cit <= it);
        EXPECT_TRUE (cit < it2);
        EXPECT_TRUE (cit <= it2);
        EXPECT_TRUE (cit < s.end ());
        EXPECT_TRUE (cit <= s.end ());

        EXPECT_TRUE (it2 > it);
        EXPECT_TRUE (it2 >= it);
        EXPECT_TRUE (s.end () > it2);
        EXPECT_TRUE (s.end () >= it2);
        EXPECT_TRUE (it2 > cit);
        EXPECT_TRUE (it2 >= cit);
    }
}

TEST (GslSpan, BeginEnd) {
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        span<int>::iterator it = s.begin ();
        span<int>::iterator it2 = std::begin (s);
        EXPECT_EQ (it, it2);

        it = s.end ();
        it2 = std::end (s);
        EXPECT_EQ (it, it2);
    }
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        auto it = s.begin ();
        auto first = it;
        EXPECT_EQ (it, first);
        EXPECT_EQ (*it, 1);

        auto beyond = s.end ();
        EXPECT_NE (it, beyond);

        EXPECT_EQ (beyond - first, 4);
        EXPECT_EQ (first - first, 0);
        EXPECT_EQ (beyond - beyond, 0);

        ++it;
        EXPECT_EQ (it - first, 1);
        EXPECT_EQ (*it, 2);
        *it = 22;
        EXPECT_EQ (*it, 22);
        EXPECT_EQ (beyond - it, 3);

        it = first;
        EXPECT_EQ (it, first);
        while (it != s.end ()) {
            *it = 5;
            ++it;
        }

        EXPECT_EQ (it, beyond);
        EXPECT_EQ (it - beyond, 0);

        for (auto & n : s) {
            EXPECT_EQ (n, 5);
        }
    }
}

TEST (GslSpan, CbeginCend) {
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        span<int>::const_iterator cit = s.begin ();
        span<int>::const_iterator cit2 = std::begin (s);
        EXPECT_EQ (cit, cit2);

        cit = s.end ();
        cit2 = std::end (s);
        EXPECT_EQ (cit, cit2);
    }
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        auto it = s.cbegin ();
        auto first = it;
        EXPECT_EQ (it, first);
        EXPECT_EQ (*it, 1);

        auto beyond = s.cend ();
        EXPECT_NE (it, beyond);

        EXPECT_EQ (beyond - first, 4);
        EXPECT_EQ (first - first, 0);
        EXPECT_EQ (beyond - beyond, 0);

        ++it;
        EXPECT_EQ (it - first, 1);
        EXPECT_EQ (*it, 2);
        EXPECT_EQ (beyond - it, 3);

        int last = 0;
        it = first;
        EXPECT_EQ (it, first);
        while (it != s.cend ()) {
            EXPECT_EQ (*it, last + 1);

            last = *it;
            ++it;
        }

        EXPECT_EQ (it, beyond);
        EXPECT_EQ (it - beyond, 0);
    }
}

TEST (GslSpan, RbeginRend) {
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        auto it = s.rbegin ();
        auto first = it;
        EXPECT_EQ (it, first);
        EXPECT_EQ (*it, 4);

        auto beyond = s.rend ();
        EXPECT_NE (it, beyond);

        EXPECT_EQ (beyond - first, 4);
        EXPECT_EQ (first - first, 0);
        EXPECT_EQ (beyond - beyond, 0);

        ++it;
        EXPECT_EQ (it - first, 1);
        EXPECT_EQ (*it, 3);
        *it = 22;
        EXPECT_EQ (*it, 22);
        EXPECT_EQ (beyond - it, 3);

        it = first;
        EXPECT_EQ (it, first);
        while (it != s.rend ()) {
            *it = 5;
            ++it;
        }

        EXPECT_EQ (it, beyond);
        EXPECT_EQ (it - beyond, 0);

        for (auto & n : s) {
            EXPECT_EQ (n, 5);
        }
    }
}

TEST (GslSpan, CrbeginCrend) {
    {
        int a[] = {1, 2, 3, 4};
        span<int> s = make_span (a);

        auto it = s.crbegin ();
        auto first = it;
        EXPECT_EQ (it, first);
        EXPECT_EQ (*it, 4);

        auto beyond = s.crend ();
        EXPECT_NE (it, beyond);

        EXPECT_EQ (beyond - first, 4);
        EXPECT_EQ (first - first, 0);
        EXPECT_EQ (beyond - beyond, 0);

        ++it;
        EXPECT_EQ (it - first, 1);
        EXPECT_EQ (*it, 3);
        EXPECT_EQ (beyond - it, 3);

        it = first;
        EXPECT_EQ (it, first);
        int last = 5;
        while (it != s.crend ()) {
            EXPECT_EQ (*it, last - 1);
            last = *it;
            ++it;
        }

        EXPECT_EQ (it, beyond);
        EXPECT_EQ (it - beyond, 0);
    }
}

TEST (GslSpan, ComparisonOperators) {
    {
        span<int> s1{nullptr};
        span<int> s2{nullptr};
        EXPECT_EQ (s1, s2);
        EXPECT_TRUE (!(s1 != s2));
        EXPECT_TRUE (!(s1 < s2));
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s1 >= s2);
        EXPECT_TRUE (s2 == s1);
        EXPECT_TRUE (!(s2 != s1));
        EXPECT_TRUE (!(s2 < s1));
        EXPECT_TRUE (s2 <= s1);
        EXPECT_TRUE (!(s2 > s1));
        EXPECT_TRUE (s2 >= s1);
    }
    {
        int arr[] = {2, 1};
        span<int> s1 = make_span (arr);
        span<int> s2 = make_span (arr);

        EXPECT_TRUE (s1 == s2);
        EXPECT_TRUE (!(s1 != s2));
        EXPECT_TRUE (!(s1 < s2));
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s1 >= s2);
        EXPECT_TRUE (s2 == s1);
        EXPECT_TRUE (!(s2 != s1));
        EXPECT_TRUE (!(s2 < s1));
        EXPECT_TRUE (s2 <= s1);
        EXPECT_TRUE (!(s2 > s1));
        EXPECT_TRUE (s2 >= s1);
    }
    {
        int arr[] = {2, 1}; // bigger

        span<int> s1{nullptr};
        span<int> s2 = make_span (arr);

        EXPECT_TRUE (s1 != s2);
        EXPECT_TRUE (s2 != s1);
        EXPECT_FALSE (s1 == s2);
        EXPECT_FALSE (s2 == s1);
        EXPECT_TRUE (s1 < s2);
        EXPECT_FALSE (s2 < s1);
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s2 <= s1));
        EXPECT_TRUE (s2 > s1);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s2 >= s1);
        EXPECT_TRUE (!(s1 >= s2));
    }
    {
        int arr1[] = {1, 2};
        int arr2[] = {1, 2};
        span<int> s1 = make_span (arr1);
        span<int> s2 = make_span (arr2);

        EXPECT_TRUE (s1 == s2);
        EXPECT_TRUE (!(s1 != s2));
        EXPECT_TRUE (!(s1 < s2));
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s1 >= s2);
        EXPECT_TRUE (s2 == s1);
        EXPECT_TRUE (!(s2 != s1));
        EXPECT_TRUE (!(s2 < s1));
        EXPECT_TRUE (s2 <= s1);
        EXPECT_TRUE (!(s2 > s1));
        EXPECT_TRUE (s2 >= s1);
    }
    {
        int arr[] = {1, 2, 3};

        span<int> s1 = {&arr[0], 2};    // shorter
        span<int> s2 = make_span (arr); // longer

        EXPECT_TRUE (s1 != s2);
        EXPECT_TRUE (s2 != s1);
        EXPECT_TRUE (!(s1 == s2));
        EXPECT_TRUE (!(s2 == s1));
        EXPECT_TRUE (s1 < s2);
        EXPECT_TRUE (!(s2 < s1));
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s2 <= s1));
        EXPECT_TRUE (s2 > s1);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s2 >= s1);
        EXPECT_TRUE (!(s1 >= s2));
    }
    {
        int arr1[] = {1, 2}; // smaller
        int arr2[] = {2, 1}; // bigger

        span<int> s1 = make_span (arr1);
        span<int> s2 = make_span (arr2);

        EXPECT_TRUE (s1 != s2);
        EXPECT_TRUE (s2 != s1);
        EXPECT_TRUE (!(s1 == s2));
        EXPECT_TRUE (!(s2 == s1));
        EXPECT_TRUE (s1 < s2);
        EXPECT_TRUE (!(s2 < s1));
        EXPECT_TRUE (s1 <= s2);
        EXPECT_TRUE (!(s2 <= s1));
        EXPECT_TRUE (s2 > s1);
        EXPECT_TRUE (!(s1 > s2));
        EXPECT_TRUE (s2 >= s1);
        EXPECT_TRUE (!(s1 >= s2));
    }
}

TEST (GslSpan, AsBytes) {
    int a[] = {1, 2, 3, 4};

    {
        span<int> s;
        auto bs = as_bytes (s);
        EXPECT_EQ (bs.length (), s.length ());
        EXPECT_EQ (bs.length (), 0);
        EXPECT_EQ (bs.size_bytes (), 0);
        EXPECT_EQ (static_cast<const void *> (bs.data ()), static_cast<const void *> (s.data ()));
        EXPECT_EQ (bs.data (), nullptr);
    }

    {
        span<int> s = make_span (a);
        auto bs = as_bytes (s);
        EXPECT_EQ (static_cast<const void *> (bs.data ()), static_cast<const void *> (s.data ()));
        EXPECT_EQ (bs.length (), s.length_bytes ());
    }
}

TEST (GslSpan, AsWriteableBytes) {
    int a[] = {1, 2, 3, 4};

    {
#ifdef CONFIRM_COMPILATION_ERRORS
        // you should not be able to get writeable bytes for const objects
        span<const int> s = a;
        EXPECT_EQ (s.length (), 4);
        span<const byte> bs = as_writeable_bytes (s);
        EXPECT_EQ (static_cast<void *> (bs.data ()), static_cast<void *> (s.data ()));
        EXPECT_EQ (bs.length (), s.length_bytes ());
#endif
    }

    {
        span<int> s;
        auto bs = as_writeable_bytes (s);
        EXPECT_EQ (bs.length (), s.length ());
        EXPECT_EQ (bs.length (), 0);
        EXPECT_EQ (bs.size_bytes (), 0);
        EXPECT_EQ (static_cast<void *> (bs.data ()), static_cast<void *> (s.data ()));
        EXPECT_EQ (bs.data (), nullptr);
    }

    {
        span<int> s = make_span (a);
        auto bs = as_writeable_bytes (s);
        EXPECT_EQ (static_cast<void *> (bs.data ()), static_cast<void *> (s.data ()));
        EXPECT_EQ (bs.length (), s.length_bytes ());
    }
}

TEST (GslSpan, FixedSizeConversions) {
    int arr[] = {1, 2, 3, 4};

    // converting to an span from an equal size array is ok
    span<int, 4> s4 = make_span (arr);
    EXPECT_EQ (s4.length (), 4);

    // converting to dynamic_range is always ok
    {
        span<int> s = s4;
        EXPECT_EQ (s.length (), s4.length ());
        static_cast<void> (s);
    }

// initialization or assignment to static span that REDUCES size is NOT ok
#ifdef CONFIRM_COMPILATION_ERRORS
    { span<int, 2> s = arr; }
    {
        span<int, 2> s2 = s4;
        static_cast<void> (s2);
    }
#endif

// even when done dynamically
#if 0
    {
        span<int> s = make_span (arr);
        auto f = [&]() {
            span<int, 2> s2 = s;
            static_cast<void>(s2);
        };
        CHECK_THROW(f(), fail_fast);
    }
#endif
    // but doing so explicitly is ok

    // you can convert statically
    {
        span<int, 2> s2 = {arr, 2};
        static_cast<void> (s2);
    }
    {
        span<int, 1> s1 = s4.first<1> ();
        static_cast<void> (s1);
    }

    // ...or dynamically
    {
        // NB: implicit conversion to span<int,1> from span<int>
        span<int, 1> s1 = s4.first (1);
        static_cast<void> (s1);
    }

// initialization or assignment to static span that requires size INCREASE is not ok.
#if 0
    int arr2[2] = {1, 2};
#endif
#ifdef CONFIRM_COMPILATION_ERRORS
    { span<int, 4> s3 = arr2; }
    {
        span<int, 2> s2 = arr2;
        span<int, 4> s4a = s2;
    }
#endif
#if 0
    {
        auto f = [&]() {
            span<int, 4> s4 = {arr2, 2};
            static_cast<void>(s4);
        };
        CHECK_THROW(f(), fail_fast);
    }
    // this should fail - we are trying to assign a small dynamic span to a fixed_size larger one
    span<int> av = arr2;
    auto f = [&]() {
        span<int, 4> s4 = av;
        static_cast<void>(s4);
    };
    CHECK_THROW(f(), fail_fast);
#endif
}
