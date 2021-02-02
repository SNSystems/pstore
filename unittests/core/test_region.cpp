//*                 _              *
//*  _ __ ___  __ _(_) ___  _ __   *
//* | '__/ _ \/ _` | |/ _ \| '_ \  *
//* | | |  __/ (_| | | (_) | | | | *
//* |_|  \___|\__, |_|\___/|_| |_| *
//*           |___/                *
//===- unittests/core/test_region.cpp -------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/core/region.hpp"
#include <vector>

#include "gmock/gmock.h"

#include "pstore/os/file.hpp"

namespace {

    class Region : public ::testing::Test {
    public:
        std::shared_ptr<std::uint8_t> make_array (std::size_t size) {
            return std::shared_ptr<std::uint8_t>{new std::uint8_t[size],
                                                 [] (std::uint8_t * p) { delete[] p; }};
        }
    };

} // namespace

TEST_F (Region, Single) {
    constexpr std::size_t size = 32;

    auto sp = this->make_array (size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, size, size);

    pstore::region::mem_based_factory factory (file, size, size);
    std::vector<std::shared_ptr<pstore::memory_mapper_base>> result = factory.init ();

    ASSERT_EQ (1U, result.size ());

    auto const & region0 = *(result.at (0));
    EXPECT_EQ (sp, region0.data ());
    EXPECT_EQ (0U, region0.offset ());
    EXPECT_EQ (size, region0.size ());
    EXPECT_TRUE (region0.is_writable ());
}

TEST_F (Region, UnderSizedFile) {
    constexpr std::size_t file_size = 16;
    constexpr std::size_t region_size = 32;

    auto sp = this->make_array (file_size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, file_size, file_size);

    pstore::region::mem_based_factory factory (file, region_size, region_size);
    auto result = factory.init ();

    ASSERT_EQ (1U, result.size ());

    auto const & region0 = *(result.at (0));
    EXPECT_EQ (sp, region0.data ());
    EXPECT_EQ (0U, region0.offset ());
    EXPECT_EQ (region_size, region0.size ());
    EXPECT_TRUE (region0.is_writable ());
}

TEST_F (Region, OneLargeOneSmallRegion) {
    constexpr std::size_t big_region_size = 32;
    constexpr std::size_t small_region_size = 16;
    constexpr std::size_t file_size = big_region_size + small_region_size;

    auto sp = this->make_array (file_size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, file_size, file_size);

    pstore::region::mem_based_factory factory (file, big_region_size, small_region_size);
    std::vector<std::shared_ptr<pstore::memory_mapper_base>> result = factory.init ();

    ASSERT_EQ (2U, result.size ()) << "The region factory did not return exactly 2 regions";
    {
        auto const & region0 = *(result.at (0));

        EXPECT_EQ (sp, region0.data ());
        EXPECT_EQ (0U, region0.offset ());
        EXPECT_EQ (big_region_size, region0.size ());
        EXPECT_TRUE (region0.is_writable ());
    }
    {
        auto const & region1 = *(result.at (1));
        std::uint8_t const * expected = sp.get () + big_region_size;
        void const * actual = region1.data ().get ();

        EXPECT_EQ (expected, actual);
        EXPECT_EQ (big_region_size, region1.offset ());
        EXPECT_EQ (small_region_size, region1.size ());
        EXPECT_TRUE (region1.is_writable ());
    }
}

TEST_F (Region, TwoSmallRegions) {
    constexpr std::size_t big_region_size = 64;
    constexpr std::size_t small_region_size = 16;
    constexpr std::size_t file_size = small_region_size * 2;

    // The region builder tries to create regions which are as large as possible (in multiple of
    // the "minimum" size, but no larger than "full" size to avoid requesting too much
    // contiguous address space.
    auto sp = this->make_array (file_size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, file_size, file_size);

    pstore::region::mem_based_factory factory (file, big_region_size, small_region_size);
    std::vector<std::shared_ptr<pstore::memory_mapper_base>> result = factory.init ();

    ASSERT_EQ (1U, result.size ()) << "The region factory did not return exactly 1 region";
    {
        auto const & region0 = *(result.at (0));

        EXPECT_EQ (sp, region0.data ());
        EXPECT_EQ (0U, region0.offset ());
        EXPECT_EQ (small_region_size * 2, region0.size ());
        EXPECT_TRUE (region0.is_writable ());
    }
}

TEST_F (Region, OneLargeOneSmallRegionReadOnly) {
    constexpr std::size_t big_region_size = 32;
    constexpr std::size_t small_region_size = 16;
    constexpr std::size_t file_size = big_region_size + small_region_size;

    auto sp = this->make_array (file_size);
    auto file =
        std::make_shared<pstore::file::in_memory> (sp, file_size, file_size, false /*writable*/);

    pstore::region::mem_based_factory factory (file, big_region_size, small_region_size);
    auto result = factory.init ();
    ASSERT_EQ (2U, result.size ()) << "The region factory did not return exactly 2 regions";
    {
        auto const & region0 = *(result.at (0));

        EXPECT_EQ (sp, region0.data ());
        EXPECT_EQ (0U, region0.offset ());
        EXPECT_EQ (big_region_size, region0.size ());
        EXPECT_FALSE (region0.is_writable ());
    }
    {
        auto const & region1 = *(result.at (1));
        std::uint8_t const * expected = sp.get () + big_region_size;
        void const * actual = region1.data ().get ();

        EXPECT_EQ (expected, actual);
        EXPECT_EQ (big_region_size, region1.offset ());
        EXPECT_EQ (small_region_size, region1.size ());
        EXPECT_FALSE (region1.is_writable ())
            << "The 2nd region of 2 should not be writable because the file is not writable";
    }
}

TEST_F (Region, OversizedFile) {
    constexpr std::size_t big_region_size = 64;
    constexpr std::size_t small_region_size = 16;
    constexpr std::size_t xxl = 8;
    constexpr std::size_t file_size = big_region_size + small_region_size + xxl;

    auto sp = this->make_array (file_size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, file_size, file_size);

    pstore::region::mem_based_factory factory (file, big_region_size, small_region_size);
    auto result = factory.init ();

    ASSERT_EQ (2U, result.size ()) << "The region factory did not return exactly 2 regions";
    {
        auto const & region0 = *(result.at (0));

        EXPECT_EQ (sp, region0.data ());
        EXPECT_EQ (0U, region0.offset ());
        EXPECT_EQ (big_region_size, region0.size ());
        EXPECT_TRUE (region0.is_writable ()) << "The first region of 0 should not be writable";
    }
    {
        auto const & region1 = *(result.at (1));
        std::uint8_t const * expected = sp.get () + big_region_size;
        void const * actual = region1.data ().get ();

        EXPECT_EQ (expected, actual);
        EXPECT_EQ (big_region_size, region1.offset ());
        EXPECT_EQ (small_region_size * 2, region1.size ());
        EXPECT_TRUE (region1.is_writable ());
    }
}

TEST_F (Region, GrowByMinimumSize) {
    constexpr std::size_t big_region_size = 64;
    constexpr std::size_t small_region_size = 16;
    constexpr std::size_t file_size = big_region_size + small_region_size;

    // Make a file which contains big-region bytes.
    auto sp = this->make_array (file_size);
    auto file = std::make_shared<pstore::file::in_memory> (sp, file_size, big_region_size);

    pstore::region::mem_based_factory factory (file, big_region_size, small_region_size);
    auto result = factory.init ();
    factory.add (&result, big_region_size, big_region_size + small_region_size);

    ASSERT_EQ (2U, result.size ()) << "The region factory did not return exactly 2 regions";
    {
        auto const & region0 = *(result.at (0));

        EXPECT_EQ (sp, region0.data ());
        EXPECT_EQ (0U, region0.offset ());
        EXPECT_EQ (big_region_size, region0.size ());
        EXPECT_TRUE (region0.is_writable ()) << "The 1st region is expected to be writable";
    }
    {
        auto const & region1 = *(result.at (1));
        std::uint8_t const * expected = sp.get () + big_region_size;
        void const * actual = region1.data ().get ();

        EXPECT_EQ (expected, actual);
        EXPECT_EQ (big_region_size, region1.offset ());
        EXPECT_EQ (small_region_size, region1.size ());
        EXPECT_TRUE (region1.is_writable ()) << "The 2nd region is expected to be writable";
    }
}
