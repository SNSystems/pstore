//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===- unittests/exchange/test_export_emit.cpp ----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/exchange/export_emit.hpp"

#include <sstream>

#include "gtest/gtest.h"

TEST (ExportEmitString, SimpleString) {
    using namespace std::string_literals;
    {
        std::ostringstream os1;
        auto const empty = ""s;
        pstore::exchange::export_ns::emit_string (os1, std::begin (empty), std::end (empty));
        EXPECT_EQ (os1.str (), R"("")");
    }
    {
        std::ostringstream os2;
        auto const hello = "hello"s;
        pstore::exchange::export_ns::emit_string (os2, std::begin (hello), std::end (hello));
        EXPECT_EQ (os2.str (), R"("hello")");
    }
}

TEST (ExportEmitString, EscapeQuotes) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"(a " b)"s;
    pstore::exchange::export_ns::emit_string (os, std::begin (str), std::end (str));
    constexpr auto * expected = R"("a \" b")";
    EXPECT_EQ (os.str (), expected);
}

TEST (ExportEmitString, EscapeBackslash) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"(\)"s;
    pstore::exchange::export_ns::emit_string (os, std::begin (str), std::end (str));
    EXPECT_EQ (os.str (), R"("\\")");
}

TEST (ExportEmitString, Multiple) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"("abc\def")"s;
    pstore::exchange::export_ns::emit_string (os, std::begin (str), std::end (str));
    constexpr auto * expected = R"("\"abc\\def\"")";
    EXPECT_EQ (os.str (), expected);
}

TEST (ExportEmitArray, Empty) {
    std::ostringstream os;
    std::vector<int> values{};
    emit_array (os, pstore::exchange::export_ns::indent{}, std::begin (values), std::end (values),
                [] (std::ostringstream & os1, pstore::exchange::export_ns::indent ind, int v) {
                    os1 << ind << v;
                });
    auto const actual = os.str ();
    EXPECT_EQ (actual, "[]");
}

TEST (ExportEmitArray, Array) {
    std::ostringstream os;
    std::vector<int> values{2, 3, 5};
    emit_array (os, pstore::exchange::export_ns::indent{}, std::begin (values), std::end (values),
                [] (std::ostringstream & os1, pstore::exchange::export_ns::indent ind, int v) {
                    os1 << ind << v;
                });
    auto const actual = os.str ();
    EXPECT_EQ (actual, "[\n  2,\n  3,\n  5\n]");
}
