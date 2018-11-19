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
//===- unittests/core/test_generation_iterator.cpp ------------------------===//
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
#include "pstore/core/crc32.hpp"
#include "pstore/core/generation_iterator.hpp"
#include "pstore/core/transaction.hpp"

#include <iterator>

// 3rd party includes
#include "gtest/gtest.h"

// Local includes
#include "check_for_error.hpp"
#include "empty_store.hpp"

namespace {
    class GenerationIterator : public EmptyStore {
    public:
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
} // namespace

TEST_F (GenerationIterator, GenerationContainerBegin) {
    this->add_transaction ();
    pstore::generation_iterator actual = pstore::generation_container (*db_).begin ();
    pstore::generation_iterator expected = pstore::generation_iterator (*db_, db_->footer_pos ());
    EXPECT_EQ (expected, actual);
}
TEST_F (GenerationIterator, GenerationContainerEnd) {
    this->add_transaction ();
    pstore::generation_iterator actual = pstore::generation_container (*db_).end ();
    pstore::generation_iterator expected =
        pstore::generation_iterator (*db_, pstore::typed_address<pstore::trailer>::null ());
    EXPECT_EQ (expected, actual);
}

TEST_F (GenerationIterator, InitialStoreIterationHasDistance1) {
    auto begin = pstore::generation_iterator (*db_, db_->footer_pos ());
    auto end = pstore::generation_iterator (*db_, pstore::typed_address<pstore::trailer>::null ());
    EXPECT_EQ (1U, std::distance (begin, end));
    EXPECT_EQ (pstore::typed_address<pstore::trailer>::make (sizeof (pstore::header)), *begin);
}

TEST_F (GenerationIterator, AddTransactionoreIterationHasDistance2) {
    this->add_transaction ();
    auto begin = pstore::generation_iterator (*db_, db_->footer_pos ());
    auto end = pstore::generation_iterator (*db_, pstore::typed_address<pstore::trailer>::null ());
    EXPECT_EQ (2U, std::distance (begin, end));
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
