//*                        _                                             *
//* __      _____  _ __ __| | __      ___ __ __ _ _ __  _ __   ___ _ __  *
//* \ \ /\ / / _ \| '__/ _` | \ \ /\ / / '__/ _` | '_ \| '_ \ / _ \ '__| *
//*  \ V  V / (_) | | | (_| |  \ V  V /| | | (_| | |_) | |_) |  __/ |    *
//*   \_/\_/ \___/|_|  \__,_|   \_/\_/ |_|  \__,_| .__/| .__/ \___|_|    *
//*                                              |_|   |_|               *
//===- unittests/command_line/test_word_wrapper.cpp -----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/command_line/word_wrapper.hpp"

#include <vector>

#include "gmock/gmock.h"

using namespace pstore::command_line;

TEST (WordWrapper, NoWrap) {
    std::string const text = "text";
    word_wrapper wr (text, text.length ());
    EXPECT_EQ (*wr, "text");
}

TEST (WordWrapper, TwoLines) {
    std::string const text = "one two";

    std::vector<std::string> lines;
    // Note this loop it hand-written to ensure that the test operations don't vary according to the
    // implementation of standard library functions.
    auto it = word_wrapper (text, 4);
    auto end = word_wrapper::end (text, 4);
    while (it != end) {
        std::string const & s = *it;
        lines.push_back (s);
        ++it;
    }
    EXPECT_THAT (lines, ::testing::ElementsAre ("one", "two"));
}

TEST (WordWrapper, LongWordShortLine) {
    std::string const text = "antidisestablishmentarianism is along word";

    std::vector<std::string> lines;
    // Note this loop it hand-written to ensure that the test operations don't vary according to the
    // implementation of standard library functions.
    auto it = word_wrapper (text, 4);
    std::string const & s = *it;
    EXPECT_EQ (s, "antidisestablishmentarianism");
}
