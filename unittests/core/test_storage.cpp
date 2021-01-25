//*      _                              *
//*  ___| |_ ___  _ __ __ _  __ _  ___  *
//* / __| __/ _ \| '__/ _` |/ _` |/ _ \ *
//* \__ \ || (_) | | | (_| | (_| |  __/ *
//* |___/\__\___/|_|  \__,_|\__, |\___| *
//*                         |___/       *
//===- unittests/core/test_storage.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/database.hpp"

#include <gtest/gtest.h>

// In "always spanning" mode request_spans_regions() ALWAYS returns true!
#ifndef PSTORE_ALWAYS_SPANNING

namespace {

    class RequestSpansRegions : public testing::Test {
    protected:
        std::shared_ptr<pstore::file::in_memory> build_new_store (std::size_t file_size);
        void allocate (pstore::database & db, std::size_t size);
    };

    std::shared_ptr<pstore::file::in_memory>
    RequestSpansRegions::build_new_store (std::size_t file_size) {
        static constexpr auto page_size = std::size_t{4096};
        auto file = std::make_shared<pstore::file::in_memory> (
            pstore::aligned_valloc (file_size, page_size), file_size);
        pstore::database::build_new_store (*file);
        return file;
    }

    void RequestSpansRegions::allocate (pstore::database & db, std::size_t size) {
        ASSERT_GT (size, db.size ());
        db.allocate (size - db.size (), 1U /*align*/);
        ASSERT_EQ (db.size (), size);
    }

} // end anonymous namespace

TEST_F (RequestSpansRegions, MinRegionSize) {
    auto const region_size = pstore::address{pstore::storage::min_region_size};
    auto file = this->build_new_store (pstore::storage::min_region_size + region_size.absolute ());

    {
        pstore::database db1{file};
        db1.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        this->allocate (db1, region_size.absolute () + 1U);
        pstore::storage const & st1 = db1.storage ();

        ASSERT_EQ (st1.regions ().size (), 2U)
            << "The allocate() should should require a second region to be created.";

        pstore::storage::region_container const & regions1 = st1.regions ();
        EXPECT_EQ (regions1[0]->size (), pstore::storage::min_region_size);
        EXPECT_EQ (regions1[0]->offset (), 0U);
        EXPECT_EQ (regions1[1]->size (), pstore::storage::min_region_size);
        EXPECT_EQ (regions1[1]->offset (), pstore::storage::min_region_size);

        EXPECT_FALSE (st1.request_spans_regions (pstore::address::null (), std::size_t{0}));
        EXPECT_FALSE (
            st1.request_spans_regions (pstore::address::null (), pstore::address::segment_size));
        EXPECT_FALSE (st1.request_spans_regions (region_size - 1U, std::size_t{1}));
        EXPECT_FALSE (st1.request_spans_regions (region_size, std::size_t{1}));
        EXPECT_TRUE (st1.request_spans_regions (region_size - 1U, std::size_t{2}));
    }
    {
        pstore::database db2{file};
        db2.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

        pstore::storage const & st2 = db2.storage ();
        ASSERT_EQ (st2.regions ().size (), 1U)
            << "On open, we create regions that are as large as possible (up to full_region_size).";

        pstore::storage::region_container const & regions2 = st2.regions ();
        EXPECT_EQ (regions2[0]->size (), pstore::storage::min_region_size * 2);
        EXPECT_EQ (regions2[0]->offset (), 0U);

        EXPECT_FALSE (
            st2.request_spans_regions (pstore::address::null (), pstore::address::segment_size));
        EXPECT_FALSE (st2.request_spans_regions (region_size - 1U, std::size_t{1}));
        EXPECT_FALSE (st2.request_spans_regions (region_size, std::size_t{1}));
        EXPECT_FALSE (st2.request_spans_regions (region_size - 1U, std::size_t{2}));
        EXPECT_FALSE (
            st2.request_spans_regions (pstore::address::null (), region_size.absolute () + 1U));
    }
}

// The FullRegionSize test is slow and can exhaust memory on some systems with tightly
// constrained memory limits (e.g. inside a docker container).
#    ifdef PSTORE_FULL_REGION_SIZE_TEST_ENABLED

TEST_F (RequestSpansRegions, FullRegionSize) {
    static constexpr auto min_region_size = pstore::storage::min_region_size;
    static constexpr auto full_region_size = pstore::storage::full_region_size;

    auto file = this->build_new_store (min_region_size + full_region_size);

    {
        pstore::database db1{file};
        db1.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

        this->allocate (db1, full_region_size + 1U);

        pstore::storage const & st1 = db1.storage ();
        ASSERT_EQ (st1.regions ().size (), 2U);
        pstore::storage::region_container const & regions1 = st1.regions ();
        EXPECT_EQ (regions1[0]->size (), min_region_size);
        EXPECT_EQ (regions1[0]->offset (), 0U);
        EXPECT_EQ (regions1[1]->size (), full_region_size);
        EXPECT_EQ (regions1[1]->offset (), min_region_size);

        EXPECT_FALSE (
            st1.request_spans_regions (pstore::address::null (), pstore::address::segment_size));
        EXPECT_FALSE (
            st1.request_spans_regions (pstore::address{min_region_size} - 1U, std::size_t{1}));
        EXPECT_FALSE (st1.request_spans_regions (pstore::address{min_region_size}, std::size_t{1}));
        EXPECT_TRUE (
            st1.request_spans_regions (pstore::address{min_region_size} - 1U, std::size_t{2}));
    }
    {
        pstore::database db2{file};
        db2.set_vacuum_mode (pstore::database::vacuum_mode::disabled);

        pstore::storage const & st2 = db2.storage ();
        ASSERT_EQ (st2.regions ().size (), 2U)
            << "On open, we create regions that are as large as possible (up to full_region_size).";

        pstore::storage::region_container const & regions2 = st2.regions ();
        EXPECT_EQ (regions2[0]->size (), full_region_size);
        EXPECT_EQ (regions2[0]->offset (), 0U);
        EXPECT_EQ (regions2[1]->size (), min_region_size);
        EXPECT_EQ (regions2[1]->offset (), full_region_size);

        EXPECT_FALSE (
            st2.request_spans_regions (pstore::address::null (), pstore::address::segment_size));
        EXPECT_FALSE (
            st2.request_spans_regions (pstore::address{full_region_size} - 1U, std::size_t{1}));
        EXPECT_FALSE (
            st2.request_spans_regions (pstore::address{full_region_size}, std::size_t{1}));
        EXPECT_TRUE (
            st2.request_spans_regions (pstore::address{full_region_size} - 1U, std::size_t{2}));
        EXPECT_FALSE (st2.request_spans_regions (pstore::address::null (), full_region_size));
        EXPECT_TRUE (st2.request_spans_regions (pstore::address::null (), full_region_size + 1U));
    }
}
#    endif // PSTORE_FULL_REGION_SIZE_TEST_ENABLED

#endif // PSTORE_ALWAYS_SPANNING
