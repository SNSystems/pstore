//*  _ _                        _ _ _   _             *
//* | (_)_ __   ___   ___ _ __ | (_) |_| |_ ___ _ __  *
//* | | | '_ \ / _ \ / __| '_ \| | | __| __/ _ \ '__| *
//* | | | | | |  __/ \__ \ |_) | | | |_| ||  __/ |    *
//* |_|_|_| |_|\___| |___/ .__/|_|_|\__|\__\___|_|    *
//*                      |_|                          *
//===- unittests/dump/test_line_splitter.cpp ------------------------------===//
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
#include "dump/line_splitter.hpp"
#include <sstream>

#include "gmock/gmock.h"

TEST (LineSplitter, SingleString) {
    value::array::container container;
    std::string str = "hello\n";

    value::line_splitter ls (&container);
    ls.append (pstore::gsl::make_span (str.data (), str.size ()));

    value::array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, SingleStringFollowedByCr) {
    value::array::container container;
    std::string str = "hello";

    value::line_splitter ls (&container);
    ls.append (pstore::gsl::make_span (str.data (), str.size ()));
    char const cr[1] = {'\n'};
    ls.append (pstore::gsl::span<char const, 1U>{cr});

    value::array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, SingleStringInTwoParts) {
    value::array::container container;
    value::line_splitter ls (&container);
    ls.append ("he");
    ls.append ("llo\n");

    value::array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, TwoStringsSingleAppend) {
    value::array::container container;
    value::line_splitter ls (&container);
    ls.append ("hello\nthere\n");

    value::array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello\n- there");
}

TEST (LineSplitter, TwoStringsInTwoParts) {
    value::array::container container;
    value::line_splitter ls (&container);
    ls.append ("hello\nth");
    ls.append ("ere\n");

    value::array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello\n- there");
}
// eof: unittests/dump/test_line_splitter.cpp
