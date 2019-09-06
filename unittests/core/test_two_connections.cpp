//*  _                                                   _   _                  *
//* | |___      _____     ___ ___  _ __  _ __   ___  ___| |_(_) ___  _ __  ___  *
//* | __\ \ /\ / / _ \   / __/ _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \/ __| *
//* | |_ \ V  V / (_) | | (_| (_) | | | | | | |  __/ (__| |_| | (_) | | | \__ \ *
//*  \__| \_/\_/ \___/   \___\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_|___/ *
//*                                                                             *
//===- unittests/core/test_two_connections.cpp ----------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

    template <typename Transaction>
    void append_int (Transaction & transaction, int v) {
        *(transaction.template alloc_rw<int> ().first) = v;
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
