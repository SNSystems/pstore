//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- unittests/pstore_mcrepo/test_fragment.cpp --------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
} // namespace

TEST_F (FragmentTest, Empty) {
    std::vector<section_content> c;
    pstore::record record = fragment::alloc (transaction_, std::begin (c), std::end (c));
    auto f = reinterpret_cast<fragment const *> (record.addr.absolute ());

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (f));
    EXPECT_EQ (0U, f->num_sections ());
}

TEST_F (FragmentTest, MakeReadOnlySection) {
    section_content rodata{section_type::ReadOnly};
    rodata.data.assign ({'r', 'o', 'd', 'a', 't', 'a'});

    std::vector<section_content> c{rodata};
    auto record = fragment::alloc (transaction_, std::begin (c), std::end (c));

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (record.addr.absolute ()));
    auto f = reinterpret_cast<fragment const *> (transaction_.get_storage ().begin ()->first);


    std::vector<std::size_t> const expected{static_cast<std::size_t> (section_type::ReadOnly)};
    auto indices = f->sections ().get_indices ();
    std::vector<std::size_t> Actual (std::begin (indices), std::end (indices));
    EXPECT_THAT (Actual, ::testing::ContainerEq (expected));

    section const & s = (*f)[section_type::ReadOnly];
    auto data_begin = std::begin (s.data ());
    auto data_end = std::end (s.data ());
    auto rodata_begin = std::begin (rodata.data);
    auto rodata_end = std::end (rodata.data);
    ASSERT_EQ (std::distance (data_begin, data_end), std::distance (rodata_begin, rodata_end));
    EXPECT_TRUE (std::equal (data_begin, data_end, rodata_begin));
    EXPECT_EQ (0U, s.ifixups ().size ());
    EXPECT_EQ (0U, s.xfixups ().size ());
}

// eof: unittests/pstore_mcrepo/test_fragment.cpp
