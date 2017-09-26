//*                                  _   _              *
//*   __ _  ___ _ __   ___ _ __ __ _| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__/ _` | __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ | | (_| | |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  \__,_|\__|_|\___/|_| |_| *
//*  |___/                                              *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===- unittests/pstore/test_generation_iterator.cpp ----------------------===//
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
#include "pstore/crc32.hpp"
#include "pstore/generation_iterator.hpp"
#include "pstore/transaction.hpp"

#include <iterator>

// 3rd party includes
#include "gtest/gtest.h"

// Local includes
#include "check_for_error.hpp"
#include "empty_store.hpp"

namespace {
    struct GenerationIterator : public EmptyStore {
        void SetUp () override {
            EmptyStore::SetUp ();
            db_.reset (new pstore::database (file_));
            db_->set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

        void TearDown () override {
            db_.reset ();
            EmptyStore::TearDown ();
        }

        void add_transaction () {
            auto transaction = pstore::begin (*db_.get ());
            *(transaction.alloc_rw<int> ().first) = 37;
            transaction.commit ();
        }

        std::unique_ptr<pstore::database> db_;
    };
}

TEST_F (GenerationIterator, GenerationContainerBegin) {
    this->add_transaction ();
    pstore::generation_iterator actual = pstore::generation_container (*db_).begin ();
    pstore::generation_iterator expected =
        pstore::generation_iterator (*db_, db_->footer_pos ());
    EXPECT_EQ (expected, actual);
}
TEST_F (GenerationIterator, GenerationContainerEnd) {
    this->add_transaction ();
    pstore::generation_iterator actual = pstore::generation_container (*db_).end ();
    pstore::generation_iterator expected =
        pstore::generation_iterator (*db_, pstore::address::null ());
    EXPECT_EQ (expected, actual);
}



TEST_F (GenerationIterator, InitialStoreIterationHasDistance1) {
    auto begin = pstore::generation_iterator (*db_, db_->footer_pos ());
    auto end = pstore::generation_iterator (*db_, pstore::address::null ());
    EXPECT_EQ (1U, std::distance (begin, end));
    EXPECT_EQ (pstore::address::make (sizeof (pstore::header)), *begin);
}

TEST_F (GenerationIterator, AddTransactionoreIterationHasDistance2) {
    this->add_transaction ();
    auto begin = pstore::generation_iterator (*db_, db_->footer_pos ());
    auto end = pstore::generation_iterator (*db_, pstore::address::null ());
    EXPECT_EQ (2U, std::distance (begin, end));
}

TEST_F (GenerationIterator, ZeroTransactionPrevPointerIsBeyondTheFileEnd) {
    {
        // Note that the 'footer' variable is scoped to guarantee that any "spanning" memory is
        // flush before we try to exercise the generation_iterator.
        auto footer = db_->getrw<pstore::trailer> (db_->footer_pos ());
        footer->a.prev_generation = pstore::address::make (file_->size ());
        footer->crc = pstore::crc32 (::pstore::gsl2::span<pstore::trailer::body> (&(footer.get ())->a, 1));
        ASSERT_TRUE (footer->crc_is_valid ());
    }

    auto fn = [&] {
        auto it = pstore::generation_iterator{*db_, db_->footer_pos ()};
        return *it;
    };
    check_for_error (fn, pstore::error_code::footer_corrupt);
}

TEST_F (GenerationIterator, ZeroTransactionSizeIsInvalid) {
    {
        auto footer = db_->getrw<pstore::trailer> (db_->footer_pos ());
        footer->a.size = file_->size ();
        footer->crc = footer->get_crc ();
        ASSERT_TRUE (footer->crc_is_valid ());
    }

    auto fn = [&] {
        auto it = pstore::generation_iterator{*db_, db_->footer_pos ()};
        return *it;
    };
    check_for_error (fn, pstore::error_code::footer_corrupt);
}


TEST_F (GenerationIterator, FooterPosWithinHeader) {
    this->add_transaction ();

    {
        auto footer0 = db_->getrw<pstore::trailer> (db_->footer_pos ());
        auto footer1 = db_->getrw<pstore::trailer> (footer0->a.prev_generation);
        footer1->a.prev_generation = pstore::address::make (sizeof (pstore::header) / 2);
        footer1->crc = footer1->get_crc ();
        ASSERT_TRUE (footer1->crc_is_valid ());
    }

    auto it = pstore::generation_iterator{*db_, db_->footer_pos ()};
    auto fn = [&it] {
        ++it;
        return *it;
    };
    check_for_error (fn, pstore::error_code::footer_corrupt);
}

TEST_F (GenerationIterator, SecondFooterHasBadSignature) {
    this->add_transaction ();

    {
        auto footer0 = db_->getrw<pstore::trailer> (db_->footer_pos ());
        auto footer1 = db_->getrw<pstore::trailer> (footer0->a.prev_generation);
        footer1->a.signature1[0] = 0;
        footer1->crc = footer1->get_crc ();
        ASSERT_TRUE (footer1->crc_is_valid ());
    }

    auto begin = pstore::generation_iterator (*db_, db_->footer_pos ());
    auto it = begin;
    auto fn = [&] {
        ++it;
        return *it;
    };
    check_for_error (fn, pstore::error_code::footer_corrupt);
}

TEST_F (GenerationIterator, PostIncrement) {
    pstore::generation_container generations (*db_);
    pstore::generation_iterator begin = generations.begin ();
    pstore::generation_iterator end = generations.end ();
    pstore::generation_iterator it = begin;
    pstore::generation_iterator old = it++;
    EXPECT_EQ (begin, old);
    EXPECT_EQ (end, it);
}
// eof: unittests/pstore/test_generation_iterator.cpp
