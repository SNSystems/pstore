//===- unittests/diff/test_revision.cpp -----------------------------------===//
//*                 _     _              *
//*  _ __ _____   _(_)___(_) ___  _ __   *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \  *
//* | | |  __/\ V /| \__ \ | (_) | | | | *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_| *
//*                                      *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
