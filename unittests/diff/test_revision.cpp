//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===- unittests/diff/test_revision.cpp -----------------------------------===//
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
#include "pstore/diff_dump/revision.hpp"

#include "pstore/support/head_revision.hpp"

#include "gtest/gtest.h"

using namespace pstore;

namespace {

    class RevisionsFixture : public ::testing::Test {
    public:
        diff_dump::revisions_type expected_revisions (revision_number r1,
                                                 revision_number r2) const {
            return std::make_pair (r1, just (r2));
        }
        static constexpr revision_number db_head_revision = 8U;
    };

    constexpr revision_number RevisionsFixture::db_head_revision;

} // namespace

TEST_F (RevisionsFixture, InitNothing) {
    diff_dump::revisions_type const expected =
        this->expected_revisions (db_head_revision, db_head_revision - 1);
    diff_dump::revisions_type const actual = diff_dump::update_revisions (
        std::make_pair (head_revision, nothing<revision_number> ()), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOneOnly) {
    constexpr auto r1 = revision_number{5};

    diff_dump::revisions_type const expected = this->expected_revisions (r1, r1 - 1);
    diff_dump::revisions_type const actual = diff_dump::update_revisions (
        std::make_pair (r1, nothing<revision_number> ()), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitZeroOnly) {
    constexpr auto r1 = revision_number{0};

    diff_dump::revisions_type const expected = this->expected_revisions (r1, r1);
    diff_dump::revisions_type const actual = diff_dump::update_revisions (
        std::make_pair (r1, nothing<revision_number> ()), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne5Two3) {
    constexpr auto r1 = revision_number{5};
    auto r2 = just (revision_number{3});

    diff_dump::revisions_type const expected = this->expected_revisions (r1, r2.value ());
    diff_dump::revisions_type const actual =
        diff_dump::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne4Two7) {
    constexpr auto r1 = revision_number{4};
    auto const r2 = just (revision_number{7});

    diff_dump::revisions_type const expected = this->expected_revisions (r2.value (), r1);
    diff_dump::revisions_type const actual =
        diff_dump::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne4Two4) {
    constexpr auto r1 = revision_number{4};
    auto r2 = just (revision_number{4});

    diff_dump::revisions_type const expected = this->expected_revisions (r1, r2.value ());
    diff_dump::revisions_type const actual =
        diff_dump::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOneHeadTwoHead) {
    constexpr auto r1 = revision_number{head_revision};
    auto r2 = just (revision_number{head_revision});

    diff_dump::revisions_type const expected =
        this->expected_revisions (db_head_revision, db_head_revision);
    diff_dump::revisions_type const actual =
        diff_dump::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}
