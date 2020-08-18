//*                             _                    _ _    *
//*   _____  ___ __   ___  _ __| |_    ___ _ __ ___ (_) |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __|  / _ \ '_ ` _ \| | __| *
//* |  __/>  <| |_) | (_) | |  | |_  |  __/ | | | | | | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__|  \___|_| |_| |_|_|\__| *
//*           |_|                                           *
//===- unittests/exchange/test_export_emit.cpp ----------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/exchange/export_emit.hpp"

#include <sstream>

#include "gtest/gtest.h"

TEST (ExportEmitString, SimpleString) {
    using namespace std::string_literals;
    {
        std::ostringstream os1;
        auto const empty = ""s;
        pstore::exchange::emit_string (os1, std::begin (empty), std::end (empty));
        EXPECT_EQ (os1.str (), R"("")");
    }
    {
        std::ostringstream os2;
        auto const hello = "hello"s;
        pstore::exchange::emit_string (os2, std::begin (hello), std::end (hello));
        EXPECT_EQ (os2.str (), R"("hello")");
    }
}

TEST (ExportEmitString, EscapeQuotes) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"(a " b)"s;
    pstore::exchange::emit_string (os, std::begin (str), std::end (str));
    constexpr auto * expected = R"("a \" b")";
    EXPECT_EQ (os.str (), expected);
}

TEST (ExportEmitString, EscapeBackslash) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"(\)"s;
    pstore::exchange::emit_string (os, std::begin (str), std::end (str));
    EXPECT_EQ (os.str (), R"("\\")");
}

TEST (ExportEmitString, Multiple) {
    using namespace std::string_literals;

    std::ostringstream os;
    auto const str = R"("abc\def")"s;
    pstore::exchange::emit_string (os, std::begin (str), std::end (str));
    constexpr auto * expected = R"("\"abc\\def\"")";
    EXPECT_EQ (os.str (), expected);
}

TEST (ExportEmitArray, Empty) {
    std::ostringstream os;
    std::vector<int> values{};
    pstore::exchange::emit_array (os, std::begin (values), std::end (values), "  ",
                                  [] (std::ostringstream & os, int v) { os << "  " << v; });
    auto const actual = os.str ();
    EXPECT_EQ (actual, "[]");
}

TEST (ExportEmitArray, Array) {
    std::ostringstream os;
    std::vector<int> values{2, 3, 5};
    pstore::exchange::emit_array (os, std::begin (values), std::end (values), "  ",
                                  [] (std::ostringstream & os, int v) { os << "  " << v; });
    auto const actual = os.str ();
    EXPECT_EQ (actual, "[\n  2,\n  3,\n  5\n  ]");
}
