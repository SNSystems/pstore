//===- unittests/mcrepo/test_debug_line_section.cpp -----------------------===//
//*      _      _                   _ _             *
//*   __| | ___| |__  _   _  __ _  | (_)_ __   ___  *
//*  / _` |/ _ \ '_ \| | | |/ _` | | | | '_ \ / _ \ *
//* | (_| |  __/ |_) | |_| | (_| | | | | | | |  __/ *
//*  \__,_|\___|_.__/ \__,_|\__, | |_|_|_| |_|\___| *
//*                         |___/                   *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/mcrepo/debug_line_section.hpp"

// System includes
#include <vector>
#include <memory>

// 3rd party includes
#include "gmock/gmock.h"

// pstore includes
#include "pstore/support/pointee_adaptor.hpp"
#include "pstore/mcrepo/fragment.hpp"

// Local includes
#include "empty_store.hpp"

namespace {

    class DebugLineSection : public EmptyStore {
    public:
        DebugLineSection ()
                : db_{this->file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        using lock_guard = std::unique_lock<mock_mutex>;
        using transaction_type = pstore::transaction<lock_guard>;

        mock_mutex mutex_;
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (DebugLineSection, RoundTrip) {
    using pstore::repo::debug_line_section_creation_dispatcher;

    constexpr auto section_type = pstore::repo::section_kind::debug_line;
    constexpr auto alignment = std::uint8_t{4};
    constexpr auto header_digest = pstore::index::digest{0x01234567U, 0x89ABCDEF};
    constexpr auto header_extent =
        pstore::make_extent (pstore::typed_address<std::uint8_t>::make (5), 7);

    pstore::repo::section_content content{section_type, alignment};
    content.data.emplace_back (std::uint8_t{11});
    content.data.emplace_back (std::uint8_t{13});

    std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers;
    dispatchers.emplace_back (
        new debug_line_section_creation_dispatcher (header_digest, header_extent, &content));

    transaction_type transaction = begin (db_, lock_guard{mutex_});
    auto fragment = pstore::repo::fragment::load (
        db_, pstore::repo::fragment::alloc (transaction,
                                            pstore::make_pointee_adaptor (dispatchers.begin ()),
                                            pstore::make_pointee_adaptor (dispatchers.end ())));
    transaction.commit ();

    auto const * const dls = fragment->atp<section_type> ();
    ASSERT_NE (dls, nullptr);
    EXPECT_EQ (dls->align (), alignment);
    EXPECT_EQ (dls->header_digest (), header_digest);
    EXPECT_EQ (dls->header_extent (), header_extent);
    EXPECT_THAT (dls->payload (), testing::ElementsAre (std::uint8_t{11}, std::uint8_t{13}));
}
