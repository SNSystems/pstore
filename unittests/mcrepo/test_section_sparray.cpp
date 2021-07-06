//===- unittests/mcrepo/test_section_sparray.cpp --------------------------===//
//*                _   _                                                     *
//*  ___  ___  ___| |_(_) ___  _ __    ___ _ __   __ _ _ __ _ __ __ _ _   _  *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  / __| '_ \ / _` | '__| '__/ _` | | | | *
//* \__ \  __/ (__| |_| | (_) | | | | \__ \ |_) | (_| | |  | | | (_| | |_| | *
//* |___/\___|\___|\__|_|\___/|_| |_| |___/ .__/ \__,_|_|  |_|  \__,_|\__, | *
//*                                       |_|                         |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/section_sparray.hpp"

#include <gmock/gmock.h>

// A type built on unique_ptr<> but where placement new is used for allocation.
namespace {

    constexpr void nop_free (void *) noexcept {}

    template <typename T>
    using placement_unique_ptr = std::unique_ptr<T, decltype (&nop_free)>;

    template <class T, class... Args>
    std::enable_if_t<!std::is_array<T>::value, placement_unique_ptr<T>>
    make_placement_unique_ptr (void * ptr, Args &&... args) {
        return placement_unique_ptr<T>{new (ptr) T (std::forward<Args> (args)...), &nop_free};
    }

} // end anonymous namespace

using namespace pstore::repo;

TEST (SectionSpArray, FrontAndBack) {
    std::vector<section_kind> const indices{section_kind::text, section_kind::data};
    using sparray = section_sparray<int>;

    std::vector<std::uint8_t> storage;
    storage.resize (sparray::size_bytes (indices.size ()));

    auto arr = make_placement_unique_ptr<sparray> (storage.data (), std::begin (indices),
                                                   std::end (indices));
    (*arr)[section_kind::text] = 17;
    (*arr)[section_kind::data] = 23;
    EXPECT_EQ (arr->front (), 17);
    EXPECT_EQ (arr->back (), 23);
}

TEST (SectionSpArray, BeginEnd) {
    std::vector<section_kind> const indices{section_kind::text, section_kind::data};
    using sparray = section_sparray<int>;

    std::vector<std::uint8_t> storage;
    storage.resize (sparray::size_bytes (indices.size ()));

    auto arr = make_placement_unique_ptr<sparray> (storage.data (), std::begin (indices),
                                                   std::end (indices));
    (*arr)[section_kind::text] = 17;
    (*arr)[section_kind::data] = 23;
    std::vector<int> c{arr->begin (), arr->end ()};
    EXPECT_THAT (c, testing::ElementsAre (17, 23));
}


TEST (SectionSpArray, HasIndex) {
    std::vector<section_kind> const indices{section_kind::text, section_kind::data};
    using sparray = section_sparray<int>;

    std::vector<std::uint8_t> storage;
    storage.resize (sparray::size_bytes (indices.size ()));

    auto arr = make_placement_unique_ptr<sparray> (storage.data (), std::begin (indices),
                                                   std::end (indices));
    EXPECT_FALSE (arr->empty ());
    EXPECT_EQ (arr->size (), indices.size ());
    EXPECT_TRUE (arr->has_index (section_kind::text));
    EXPECT_TRUE (arr->has_index (section_kind::data));
    EXPECT_FALSE (arr->has_index (section_kind::read_only));
}

TEST (SectionSpArray, SizeBytes) {
    std::vector<section_kind> const indices{section_kind::text, section_kind::data};

    using sparray = section_sparray<int>;
    auto const size = sparray::size_bytes (indices.size ());

    std::vector<std::uint8_t> storage;
    storage.resize (size);

    auto arr = make_placement_unique_ptr<sparray> (storage.data (), std::begin (indices),
                                                   std::end (indices));
    EXPECT_EQ (size, arr->size_bytes ());
}

TEST (SectionSpArray, IndicesBeginEnd) {
    std::vector<section_kind> const indices{section_kind::text, section_kind::data};
    using sparray = section_sparray<int>;

    std::vector<std::uint8_t> storage;
    storage.resize (sparray::size_bytes (indices.size ()));

    auto arr = make_placement_unique_ptr<sparray> (storage.data (), std::begin (indices),
                                                   std::end (indices));
    std::vector<section_kind> c{arr->get_indices ().begin (), arr->get_indices ().end ()};
    EXPECT_THAT (c, testing::ElementsAre (section_kind::text, section_kind::data));
}
