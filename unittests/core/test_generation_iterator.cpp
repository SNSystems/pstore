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
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#include "pstore/core/generation_iterator.hpp"
// 3rd party includes
#include <gtest/gtest.h>
// pstore includes
#include "pstore/core/transaction.hpp"
// Local includes
#include "empty_store.hpp"

using pstore::generation_container;
using pstore::generation_iterator;

namespace {

    class GenerationIterator : public EmptyStore {
    public:
        GenerationIterator ()
                : EmptyStore{}
                , db_{this->file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        void add_transaction () {
            auto transaction = pstore::begin (db_);
            *(transaction.alloc_rw<int> ().first) = 37;
            transaction.commit ();
        }

        pstore::database & db () noexcept { return db_; }

        using trailer_address = pstore::typed_address<pstore::trailer>;

    private:
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (GenerationIterator, GenerationContainerBegin) {
    this->add_transaction ();

    auto & d = this->db ();
    generation_iterator const actual = generation_container{d}.begin ();
    generation_iterator const expected = generation_iterator{&d, d.footer_pos ()};
    EXPECT_EQ (expected, actual);
}
TEST_F (GenerationIterator, GenerationContainerEnd) {
    this->add_transaction ();

    auto & d = this->db ();
    generation_iterator const actual = generation_container{d}.end ();
    generation_iterator const expected = generation_iterator{&d, trailer_address::null ()};
    EXPECT_EQ (expected, actual);
}

TEST_F (GenerationIterator, InitialStoreIterationHasDistance1) {
    auto & d = this->db ();
    auto const begin = generation_iterator{&d, d.footer_pos ()};
    auto const end = generation_iterator{&d, trailer_address::null ()};
    EXPECT_EQ (1U, std::distance (begin, end));
    EXPECT_EQ (trailer_address::make (pstore::leader_size), *begin);
}

TEST_F (GenerationIterator, AddTransactionoreIterationHasDistance2) {
    this->add_transaction ();

    auto & d = this->db ();
    auto const begin = generation_iterator{&d, d.footer_pos ()};
    auto const end = generation_iterator{&d, trailer_address::null ()};
    EXPECT_EQ (2U, std::distance (begin, end));
}

TEST_F (GenerationIterator, PostIncrement) {
    auto & d = this->db ();
    generation_container generations (d);
    generation_iterator const begin = generations.begin ();
    generation_iterator const end = generations.end ();
    generation_iterator it = begin;
    generation_iterator const old = it++;
    EXPECT_EQ (begin, old);
    EXPECT_EQ (end, it);
}
