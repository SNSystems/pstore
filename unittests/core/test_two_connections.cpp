//===- unittests/core/test_two_connections.cpp ----------------------------===//
//*  _                                                   _   _                  *
//* | |___      _____     ___ ___  _ __  _ __   ___  ___| |_(_) ___  _ __  ___  *
//* | __\ \ /\ / / _ \   / __/ _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \/ __| *
//* | |_ \ V  V / (_) | | (_| (_) | | | | | | |  __/ (__| |_| | (_) | | | \__ \ *
//*  \__| \_/\_/ \___/   \___\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_|___/ *
//*                                                                             *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <cstdint>
#include <memory>
#include <numeric>

// 3rd party includes
#include "gtest/gtest.h"

// pstore includes
#include "pstore/core/transaction.hpp"
#include "pstore/os/memory_mapper.hpp"

// Local private includes
#include "empty_store.hpp"



namespace {

    class db_file {
    public:
        static std::size_t constexpr file_size = pstore::storage::min_region_size * 2;
        db_file ();
        std::shared_ptr<pstore::file::in_memory> file () { return file_; }

    private:
        std::shared_ptr<std::uint8_t> buffer_;
        std::shared_ptr<pstore::file::in_memory> file_;
    };

    std::size_t constexpr db_file::file_size;

    db_file::db_file ()
            : buffer_ (pstore::aligned_valloc (file_size, 4096U))
            , file_ (std::make_shared<pstore::file::in_memory> (buffer_, file_size)) {

        pstore::database::build_new_store (*file_);
    }


    class TwoConnections : public ::testing::Test {
    public:
        TwoConnections ()
                : file ()
                , first (file.file ())
                , second (file.file ()) {

            first.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
            second.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        db_file file;
        pstore::database first;
        pstore::database second;
    };

    void append_int (pstore::transaction_base & transaction, int v) {
        *(transaction.alloc_rw<int> ().first) = v;
    }

} // end anonymous namespace

TEST_F (TwoConnections, CommitToFirstConnectionDoesNotAffectFooterPosForSecond) {
    ASSERT_EQ (pstore::leader_size, second.footer_pos ().absolute ());
    {
        auto transaction = pstore::begin (first);
        append_int (transaction, 1);
        transaction.commit ();
    }
    EXPECT_GE (first.footer_pos ().absolute (),
               pstore::leader_size + sizeof (int) + sizeof (pstore::trailer));
    EXPECT_EQ (second.footer_pos ().absolute (), pstore::leader_size);
}

TEST_F (TwoConnections, SyncOnSecondConnectionUpdatesFooterPos) {
    {
        auto transaction = pstore::begin (first);
        append_int (transaction, 1);
        transaction.commit ();
    }
    second.sync ();
    EXPECT_EQ (first.footer_pos (), second.footer_pos ());
    second.sync (0U);
    EXPECT_EQ (pstore::leader_size, second.footer_pos ().absolute ());
    second.sync (1U);
    EXPECT_EQ (first.footer_pos (), second.footer_pos ());
    second.sync (0U);
    EXPECT_EQ (pstore::leader_size, second.footer_pos ().absolute ());
}

TEST_F (TwoConnections, SyncOnSecondConnectionMapsAdditionalSpace) {
    using ::testing::ContainerEq;

    // The first connection write > min_region_size bytes in a transaction.
    std::vector<std::uint8_t> buffer1 (pstore::storage::min_region_size + 1);
    std::iota (std::begin (buffer1), std::end (buffer1), std::uint8_t{0});

    pstore::extent<std::uint8_t> t1;
    {
        auto transaction = pstore::begin (first);
        {
            t1.size = buffer1.size ();
            std::shared_ptr<std::uint8_t> ptr;
            std::tie (ptr, t1.addr) = transaction.alloc_rw<std::uint8_t> (t1.size);
            std::memcpy (ptr.get (), buffer1.data (), buffer1.size ());
        }

        transaction.commit ();
    }

    second.sync ();
    auto r = second.getro (t1);
    std::vector<std::uint8_t> buffer2 (r.get (), r.get () + t1.size);
    EXPECT_THAT (buffer2, ContainerEq (buffer1));
}
