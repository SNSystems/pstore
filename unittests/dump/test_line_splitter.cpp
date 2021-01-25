//*  _ _                        _ _ _   _             *
//* | (_)_ __   ___   ___ _ __ | (_) |_| |_ ___ _ __  *
//* | | | '_ \ / _ \ / __| '_ \| | | __| __/ _ \ '__| *
//* | | | | | |  __/ \__ \ |_) | | | |_| ||  __/ |    *
//* |_|_|_| |_|\___| |___/ .__/|_|_|\__|\__\___|_|    *
//*                      |_|                          *
//===- unittests/dump/test_line_splitter.cpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/dump/line_splitter.hpp"
#include <iterator>
#include <sstream>

#include "gmock/gmock.h"

using namespace std::string_literals;

TEST (ExpandTabs, Empty) {
    std::string in;
    std::string out;
    pstore::dump::expand_tabs (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "");
}

TEST (ExpandTabs, NoTabs) {
    auto const in = "a b"s;
    std::string out;
    pstore::dump::expand_tabs (std::begin (in), std::end (in), std::back_inserter (out));
    EXPECT_EQ (out, "a b");
}

TEST (ExpandTabs, SingleTab) {
    {
        auto const in1 = "a\tb"s;
        std::string out1;
        pstore::dump::expand_tabs (std::begin (in1), std::end (in1), std::back_inserter (out1), 8U);
        EXPECT_EQ (out1, "a       b");
    }
    {
        std::string const in2 = "12345678\t12345678";
        std::string out2;
        pstore::dump::expand_tabs (std::begin (in2), std::end (in2), std::back_inserter (out2), 8U);
        EXPECT_EQ (out2, "12345678        12345678");
    }
}

TEST (ExpandTabs, TwoTabs) {
    {
        auto const in1 = "a\tb\tc"s;
        std::string out1;
        pstore::dump::expand_tabs (std::begin (in1), std::end (in1), std::back_inserter (out1), 8U);
        EXPECT_EQ (out1, "a       b       c");
    }
    {
        auto const in2 = "\t1234567\t12345678"s;
        std::string out2;
        pstore::dump::expand_tabs (std::begin (in2), std::end (in2), std::back_inserter (out2), 8U);
        EXPECT_EQ (out2, "        1234567 12345678");
    }
}


TEST (LineSplitter, SingleString) {
    using namespace pstore::dump;

    array::container container;
    auto const str = "hello\n"s;

    line_splitter ls (&container);
    ls.append (pstore::gsl::make_span (str));

    array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, SingleStringFollowedByCr) {
    using namespace pstore::dump;

    array::container container;
    std::string const str = "hello";

    line_splitter ls (&container);
    ls.append (pstore::gsl::make_span (str));
    char const cr[1] = {'\n'};
    ls.append (pstore::gsl::span<char const, 1U>{cr});

    array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, SingleStringInTwoParts) {
    using namespace pstore::dump;

    array::container container;
    line_splitter ls (&container);
    ls.append ("he"s);
    ls.append ("llo\n"s);

    array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello");
}

TEST (LineSplitter, TwoStringsSingleAppend) {
    using namespace pstore::dump;

    array::container container;
    line_splitter ls (&container);
    ls.append ("hello\nthere\n"s);

    array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello\n- there");
}

TEST (LineSplitter, TwoStringsInTwoParts) {
    using namespace pstore::dump;

    array::container container;
    line_splitter ls (&container);
    ls.append ("hello\nth"s);
    ls.append ("ere\n"s);

    array arr (std::move (container));
    std::ostringstream out;
    arr.write (out);
    EXPECT_EQ (out.str (), "\n- hello\n- there");
}
