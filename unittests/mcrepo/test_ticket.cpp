//*  _   _      _        _    *
//* | |_(_) ___| | _____| |_  *
//* | __| |/ __| |/ / _ \ __| *
//* | |_| | (__|   <  __/ |_  *
//*  \__|_|\___|_|\_\___|\__| *
//*                           *
//===- unittests/mcrepo/test_ticket.cpp -----------------------------------===//
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

#include "pstore/mcrepo/ticket.hpp"
#include "transaction.hpp"

using namespace pstore::repo;


namespace {
    class TicketTest : public ::testing::Test {
    protected:
        transaction transaction_;

        using string_address = pstore::typed_address<pstore::indirect_string>;
        static constexpr string_address indirect_string_address (std::uint64_t x) {
            return string_address{pstore::address{x}};
        }
    };

} // namespace

TEST_F (TicketTest, Empty) {
    std::vector<ticket_member> m;
    pstore::extent<ticket> extent = ticket::alloc (transaction_, indirect_string_address (0U), m);
    auto t = reinterpret_cast<ticket const *> (extent.addr.absolute ());

    assert (transaction_.get_storage ().begin ()->first ==
            reinterpret_cast<std::uint8_t const *> (t));
    assert (transaction_.get_storage ().begin ()->second.get () ==
            reinterpret_cast<std::uint8_t const *> (t));
    EXPECT_EQ (0U, t->size ());
    EXPECT_TRUE (t->empty ());
}

TEST_F (TicketTest, SingleMember) {
    constexpr auto output_file_path = indirect_string_address (64U);
    constexpr auto digest = pstore::index::digest{28U};
    constexpr auto name = indirect_string_address (32U);
    constexpr auto linkage = pstore::repo::linkage_type::external;

    ticket_member sm{digest, name, linkage};

    std::vector<ticket_member> v{sm};
    ticket::alloc (transaction_, output_file_path, v);

    auto t = reinterpret_cast<ticket const *> (transaction_.get_storage ().begin ()->first);

    EXPECT_EQ (1U, t->size ());
    EXPECT_FALSE (t->empty ());
    EXPECT_EQ (output_file_path, t->path ());
    EXPECT_EQ (sizeof (ticket), t->size_bytes ());
    EXPECT_EQ (digest, (*t)[0].digest.low ());
    EXPECT_EQ (name, (*t)[0].name);
    EXPECT_EQ (linkage, (*t)[0].linkage);
}

TEST_F (TicketTest, MultipleMembers) {
    constexpr auto output_file_path = indirect_string_address (32U);
    constexpr auto digest1 = pstore::index::digest{128U};
    constexpr auto digest2 = pstore::index::digest{144U};
    constexpr auto name = indirect_string_address (16U);
    constexpr auto linkage = pstore::repo::linkage_type::external;

    ticket_member mm1{digest1, name, linkage};
    ticket_member mm2{digest2, name + 24U, linkage};

    std::vector<ticket_member> v{mm1, mm2};
    ticket::alloc (transaction_, output_file_path, v);

    auto t = reinterpret_cast<ticket const *> (transaction_.get_storage ().begin ()->first);

    EXPECT_EQ (2U, t->size ());
    EXPECT_FALSE (t->empty ());
    EXPECT_EQ (80U, t->size_bytes ());
    for (auto const & m : *t) {
        EXPECT_EQ (linkage, m.linkage);
    }
}

