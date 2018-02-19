//*      _ _      *
//*   __| | |__   *
//*  / _` | '_ \  *
//* | (_| | |_) | *
//*  \__,_|_.__/  *
//*               *
//===- unittests/dump/test_db.cpp -----------------------------------------===//
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

#include "dump/db_value.hpp"

#include <cctype>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "split.hpp"

namespace {
    class Address : public ::testing::Test {
    public:
        Address ()
                : old_expanded_{::pstore::dump::address::get_expanded ()} {}
        ~Address () { ::pstore::dump::address::set_expanded (old_expanded_); }

    private:
        bool old_expanded_;
    };
} // namespace

TEST_F (Address, ExpandedNull) {
    using ::testing::ElementsAre;
    using namespace ::pstore::dump;

    address::set_expanded (true);
    value_ptr obj = make_value (pstore::address::null ());

    std::ostringstream out;
    obj->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (1U, lines.size ());
    EXPECT_THAT (split_tokens (lines.at (0)),
                 ElementsAre ("{", "segment:", "0x0,", "offset:", "0x0", "}"));
}

TEST_F (Address, SimplifiedNull) {
    using ::testing::ElementsAre;
    using namespace ::pstore::dump;

    address::set_expanded (false);
    value_ptr obj = make_value (pstore::address::null ());

    std::ostringstream out;
    obj->write (out);
    EXPECT_EQ (out.str (), "0x0");
}


TEST (Database, Extent) {
    using ::testing::ElementsAre;
    std::ostringstream out;

    pstore::dump::value_ptr addr = pstore::dump::make_value (pstore::extent{});
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (1U, lines.size ());
    EXPECT_THAT (split_tokens (lines.at (0)),
                 ElementsAre ("{", "addr:", "0x0,", "size:", "0x0", "}"));
}

TEST (Database, Header) {
    using ::testing::_;
    using ::testing::ElementsAre;

    std::ostringstream out;
    pstore::dump::value_ptr addr = pstore::dump::make_value (pstore::header{});
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (6U, lines.size ());
    EXPECT_THAT (split_tokens (lines.at (0)),
                 ElementsAre ("signature1", ":", "[", "0x70,", "0x53,", "0x74,", "0x72", "]"));
    EXPECT_THAT (split_tokens (lines.at (1)), ElementsAre ("signature2", ":", "0x507ffff"));
    EXPECT_THAT (split_tokens (lines.at (2)),
                 ElementsAre ("version", ":", "[", "0x0,", "0x1", "]"));
    EXPECT_THAT (split_tokens (lines.at (3)), ElementsAre ("uuid", ":", _));
    EXPECT_THAT (split_tokens (lines.at (4)), ElementsAre ("crc", ":", _));
    EXPECT_THAT (split_tokens (lines.at (5)), ElementsAre ("footer_pos", ":", "0x0"));
}

TEST (Database, Trailer) {
    using ::testing::_;
    using ::testing::ElementsAre;
    using ::testing::ElementsAreArray;

    std::ostringstream out;
    pstore::dump::value_ptr addr = pstore::dump::make_value (pstore::trailer{}, false /*no_times*/);
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (8U, lines.size ());

    auto line = 0U;
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAreArray (std::vector<std::string>{"signature1", ":", "[", "0x68,",
                                                            "0x50,", "0x50,", "0x79,", "0x66,",
                                                            "0x6f,", "0x6f,", "0x54", "]"}));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("generation", ":", "0x0"));
    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("size", ":", "0x0"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("time", ":", "1970-01-01T00:00:00Z"));

    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("prev_generation", ":", "0x0"));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAre ("indices", ":", "[", "0x0,", "0x0,", "0x0,", "0x0", "]"));

    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("crc", ":", _));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAreArray (std::vector<std::string>{"signature2", ":", "[", "0x68,",
                                                            "0x50,", "0x50,", "0x79,", "0x54,",
                                                            "0x61,", "0x69,", "0x6c", "]"}));
}

// eof: unittests/dump/test_db.cpp
