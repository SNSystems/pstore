//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===- unittests/diff/test_revision.cpp -----------------------------------===//
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
#include "pstore/diff/revision.hpp"
#include "pstore/support/head_revision.hpp"
#include "gtest/gtest.h"

using namespace pstore;

namespace {
    struct RevisionsFixture : public ::testing::Test {
        diff::revisions_type expected_revisions (diff::revision_number r1,
                                                 diff::revision_number r2) const {
            return std::make_pair (r1, just (r2));
        }
        static constexpr diff::revision_number db_head_revision = 8;
    };

    constexpr diff::revision_number RevisionsFixture::db_head_revision;
} // namespace

TEST_F (RevisionsFixture, InitNothing) {

    diff::revisions_type const expected =
        this->expected_revisions (db_head_revision, db_head_revision - 1);
    diff::revisions_type const actual = diff::update_revisions (
        std::make_pair (head_revision, nothing<diff::revision_number> ()), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOneOnly) {
    constexpr auto r1 = diff::revision_number{5};

    diff::revisions_type const expected = this->expected_revisions (r1, r1 - 1);
    diff::revisions_type const actual = diff::update_revisions (
        std::make_pair (r1, nothing<diff::revision_number> ()), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne5Two3) {
    constexpr auto r1 = diff::revision_number{5};
    auto r2 = just (diff::revision_number{3});

    diff::revisions_type const expected = this->expected_revisions (r1, r2);
    diff::revisions_type const actual =
        diff::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne4Two7) {
    constexpr auto r1 = diff::revision_number{4};
    auto r2 = just (diff::revision_number{7});

    diff::revisions_type const expected = this->expected_revisions (r2.value (), r1);
    diff::revisions_type const actual =
        diff::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOne4Two4) {
    constexpr auto r1 = diff::revision_number{4};
    auto r2 = just (diff::revision_number{4});

    diff::revisions_type const expected = this->expected_revisions (r1, r2.value ());
    diff::revisions_type const actual =
        diff::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

TEST_F (RevisionsFixture, InitOneHeadTwoHead) {
    constexpr auto r1 = diff::revision_number{head_revision};
    auto r2 = just (diff::revision_number{head_revision});

    diff::revisions_type const expected =
        this->expected_revisions (db_head_revision, db_head_revision);
    diff::revisions_type const actual =
        diff::update_revisions (std::make_pair (r1, r2), db_head_revision);

    EXPECT_EQ (expected, actual);
}

