//===- unittests/dump/test_db.cpp -----------------------------------------===//
//*      _ _      *
//*   __| | |__   *
//*  / _` | '_ \  *
//* | (_| | |_) | *
//*  \__,_|_.__/  *
//*               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/dump/db_value.hpp"

// Standard library includes
#include <cctype>
#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

// 3rd party includes
#include <gmock/gmock.h>

// Local includes
#include "split.hpp"

namespace {

    class Address : public ::testing::Test {
    public:
        Address ()
                : old_expanded_{::pstore::dump::address::get_expanded ()} {}
        ~Address () override { ::pstore::dump::address::set_expanded (old_expanded_); }

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

TEST_F (Address, Typed) {
    using ::testing::ElementsAre;
    using namespace ::pstore::dump;

    address::set_expanded (false);
    value_ptr obj = make_value (pstore::typed_address<char>::null ());

    std::ostringstream out;
    obj->write (out);
    EXPECT_EQ (out.str (), "0x0");
}

TEST (Database, Extent) {
    using ::testing::ElementsAre;
    std::ostringstream out;

    pstore::dump::value_ptr addr = pstore::dump::make_value (pstore::extent<char>{});
    addr->write (out);

    auto const lines = split_lines (out.str ());
    ASSERT_EQ (1U, lines.size ());
    EXPECT_THAT (split_tokens (lines.at (0)),
                 ElementsAre ("{", "addr:", "0x0,", "size:", "0x0", "}"));
}

namespace {

    template <typename Int>
    std::string to_hex_string (Int v) {
        std::ostringstream os;
        os << std::hex << "0x" << v;
        return os.str ();
    }

} // end anonymous namespace


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
                 ElementsAre ("version", ":", "[",
                              to_hex_string (pstore::header::major_version) + ",",
                              to_hex_string (pstore::header::minor_version), "]"));
    EXPECT_THAT (split_tokens (lines.at (3)), ElementsAre ("id", ":", _));
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
    EXPECT_THAT (
        split_tokens (lines.at (line++)),
        ElementsAre ("indices", ":", "[", "0x0,", "0x0,", "0x0,", "0x0,", "0x0,", "0x0", "]"));

    EXPECT_THAT (split_tokens (lines.at (line++)), ElementsAre ("crc", ":", _));
    EXPECT_THAT (split_tokens (lines.at (line++)),
                 ElementsAreArray (std::vector<std::string>{"signature2", ":", "[", "0x68,",
                                                            "0x50,", "0x50,", "0x79,", "0x54,",
                                                            "0x61,", "0x69,", "0x6c", "]"}));
}
