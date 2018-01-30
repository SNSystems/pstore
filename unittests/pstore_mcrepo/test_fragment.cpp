//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- unittests/pstore_mcrepo/test_fragment.cpp --------------------------===//
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

#include "pstore_mcrepo/fragment.hpp"
#include "gmock/gmock.h"
#include <memory>
#include <vector>

#include "transaction.hpp"

using namespace pstore::repo;


namespace {
    class FragmentTest : public ::testing::Test {
    protected:
        transaction transaction_;
    };
} // anonymous namespace

TEST_F (FragmentTest, Empty) {
    std::vector<section_content> c;
    pstore::extent extent = fragment::alloc (transaction_, std::begin (c), std::end (c));
    auto f = reinterpret_cast<fragment const *> (extent.addr.absolute ());

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (f));
    EXPECT_EQ (0U, f->num_sections ());
}

TEST_F (FragmentTest, MakeReadOnlySection) {
    std::vector<section_content> c;
    c.emplace_back (section_type::read_only, std::uint8_t{2} /*alignment*/);
    section_content & rodata = c.back ();
    rodata.data.assign ({'r', 'o', 'd', 'a', 't', 'a'});
    auto extent = fragment::alloc (transaction_, std::begin (c), std::end (c));

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);


    std::vector<std::size_t> const expected{static_cast<std::size_t> (section_type::read_only)};
    auto indices = f->sections ().get_indices ();
    std::vector<std::size_t> Actual (std::begin (indices), std::end (indices));
    EXPECT_THAT (Actual, ::testing::ContainerEq (expected));

    section const & s = (*f)[section_type::read_only];
    auto data_begin = std::begin (s.data ());
    auto data_end = std::end (s.data ());
    auto rodata_begin = std::begin (rodata.data);
    auto rodata_end = std::end (rodata.data);
    ASSERT_EQ (std::distance (data_begin, data_end), std::distance (rodata_begin, rodata_end));
    EXPECT_TRUE (std::equal (data_begin, data_end, rodata_begin));
    EXPECT_EQ (2U, s.align ());
    EXPECT_EQ (0U, s.ifixups ().size ());
    EXPECT_EQ (0U, s.xfixups ().size ());
}

TEST_F (FragmentTest, MakeTextSectionWithFixups) {
    using ::testing::ElementsAreArray;
    using ::testing::ElementsAre;
    using section_type = pstore::repo::section_type;

    std::vector<std::uint8_t> const original{'t', 'e', 'x', 't'};

    std::vector<section_content> c;
    c.emplace_back (section_type::text, std::uint8_t{4} /*alignment*/);
    {
        // Build the text section's contents and fixups.
        section_content & text = c.back ();
        text.data.assign (std::begin (original), std::end (original));
        text.ifixups.emplace_back (internal_fixup{section_type::text, 1, 1, 1});
        text.ifixups.emplace_back (internal_fixup{section_type::data, 2, 2, 2});
        text.xfixups.emplace_back (external_fixup{pstore::address{3}, 3, 3, 3});
        text.xfixups.emplace_back (external_fixup{pstore::address{4}, 4, 4, 4});
        text.xfixups.emplace_back (external_fixup{pstore::address{5}, 5, 5, 5});
    }

    auto extent = fragment::alloc (transaction_, std::begin (c), std::end (c));

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);


    std::vector<std::size_t> const expected{static_cast<std::size_t> (section_type::text)};
    auto indices = f->sections ().get_indices ();
    std::vector<std::size_t> actual (std::begin (indices), std::end (indices));
    EXPECT_THAT (actual, ::testing::ContainerEq (expected));

    section const & s = (*f)[section_type::text];
    EXPECT_EQ (4U, s.align ());
    EXPECT_EQ (4U, s.data ().size ());
    EXPECT_EQ (2U, s.ifixups ().size ());
    EXPECT_EQ (3U, s.xfixups ().size ());

    EXPECT_THAT (s.data (), ElementsAreArray (original));
    EXPECT_THAT (s.ifixups (), ElementsAre (internal_fixup{section_type::text, 1, 1, 1},
                                            internal_fixup{section_type::data, 2, 2, 2}));
    EXPECT_THAT (s.xfixups (), ElementsAre (external_fixup{pstore::address{3}, 3, 3, 3},
                                            external_fixup{pstore::address{4}, 4, 4, 4},
                                            external_fixup{pstore::address{5}, 5, 5, 5}));
}

namespace pstore {
    namespace repo {

        // An implementation of the gtest PrintTo function to avoid ambiguity between the test
        // harness's function and our template implementation of operator<<.
        void PrintTo (section_type st, ::std::ostream * os) {
            *os << st;
        }

    } // namespace repo
} // namespace pstore

TEST_F (FragmentTest, TwoSections) {
    {
        std::vector<section_content> c;
        c.emplace_back (section_type::read_only, std::uint8_t{1} /*alignment*/);
        c.emplace_back (section_type::thread_data, std::uint8_t{2} /*alignment*/);
        ASSERT_LT (static_cast<int> (c.at (0).type), static_cast<int> (c.at (1).type));

        section_content & rodata = c.at (0);
        ASSERT_EQ (rodata.type, section_type::read_only);
        rodata.data.assign ({'r', 'o', 'd', 'a', 't', 'a'});

        section_content & tls = c.at (1);
        EXPECT_EQ (tls.type, section_type::thread_data);
        tls.data.assign ({'t', 'l', 's'});

        auto const extent = fragment::alloc (transaction_, std::begin (c), std::end (c));
        ASSERT_EQ (transaction_.get_storage ().begin ()->first,
                   reinterpret_cast<std::uint8_t const *> (extent.addr.absolute ()));
    }
    {
        auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);
        std::vector<std::size_t> const expected{
            static_cast<std::size_t> (section_type::read_only),
            static_cast<std::size_t> (section_type::thread_data)};
        auto indices = f->sections ().get_indices ();
        std::vector<std::size_t> Actual (std::begin (indices), std::end (indices));
        EXPECT_THAT (Actual, ::testing::ContainerEq (expected));

        section const & rodata = (*f)[section_type::read_only];
        section const & tls = (*f)[section_type::thread_data];
        EXPECT_LT (rodata.data ().begin (), tls.data ().begin ());
    }
}

// eof: unittests/pstore_mcrepo/test_fragment.cpp
