//*  _           _ _               _         _        _              *
//* (_)_ __   __| (_)_ __ ___  ___| |_   ___| |_ _ __(_)_ __   __ _  *
//* | | '_ \ / _` | | '__/ _ \/ __| __| / __| __| '__| | '_ \ / _` | *
//* | | | | | (_| | | | |  __/ (__| |_  \__ \ |_| |  | | | | | (_| | *
//* |_|_| |_|\__,_|_|_|  \___|\___|\__| |___/\__|_|  |_|_| |_|\__, | *
//*                                                           |___/  *
//===- unittests/pstore/test_indirect_string.cpp --------------------------===//
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

#include "pstore/core/indirect_string.hpp"
#include <gtest/gtest.h>

#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/serialize/types.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {

    pstore::shared_sstring_view make_shared_sstring_view (char const * s) {
        auto const length = std::strlen (s);
        auto ptr = std::shared_ptr<char> (new char[length], [](char * p) { delete[] p; });
        std::copy (s, s + length, ptr.get ());
        return {ptr, length};
    }


    class IndirectString : public EmptyStore {
    public:
        IndirectString ()
                : db_{file_} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (IndirectString, InMemoryEquality) {
    auto const view = make_shared_sstring_view ("body");
    pstore::indirect_string const x (db_, &view);
    pstore::indirect_string const y (db_, &view);
    EXPECT_EQ (x.as_string_view (), "body");
    EXPECT_TRUE (x == y);
    EXPECT_FALSE (x != y);
}


TEST_F (IndirectString, StoreRefToHeapRoundTrip) {
    constexpr auto str = "string";
    auto const sstring = make_shared_sstring_view (str);

    auto const pointer_addr = [this, &sstring]() -> pstore::address {
        // Create a transaction
        mock_mutex mutex;
        auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});

        // Construct the indirect string and write it to the store.
        pstore::indirect_string indirect{db_, &sstring};
        auto const pointer_addr = pstore::serialize::write (
            pstore::serialize::archive::make_writer (transaction), indirect);
        EXPECT_EQ (transaction.size (), sizeof (pstore::address));

        transaction.commit ();
        return pointer_addr;
    }();

    auto const ind2 = pstore::serialize::read<pstore::indirect_string> (
        pstore::serialize::archive::make_reader (db_, pointer_addr));
    EXPECT_EQ (ind2.as_string_view (), make_shared_sstring_view (str));
}

TEST_F (IndirectString, StoreRoundTrip) {
    constexpr auto str = "string";

    auto const pointer_addr = [this, str]() -> pstore::address {
        // Create a transaction.
        mock_mutex mutex;
        auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});

        // Construct the string and the indirect string. Write the indirect pointer to the store.
        pstore::shared_sstring_view const sstring = make_shared_sstring_view (str);
        pstore::indirect_string indirect{db_, &sstring};
        auto const pointer_addr = pstore::serialize::write (
            pstore::serialize::archive::make_writer (transaction), indirect);
        EXPECT_EQ (transaction.size (), sizeof (pstore::address));

        // Now the body of the string (and patch the pointer).
        pstore::indirect_string::write_body_and_patch_address (transaction, sstring, pointer_addr);

        // Commit the transaction.
        transaction.commit ();
        return pointer_addr;
    }();

    auto const ind2 = pstore::serialize::read<pstore::indirect_string> (
        pstore::serialize::archive::make_reader (db_, pointer_addr));
    EXPECT_EQ (ind2.as_string_view (), make_shared_sstring_view (str));
}


namespace {

    class IndirectStringAdder : public EmptyStore {
    public:
        IndirectStringAdder ()
                : db_{file_} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        pstore::database db_;
    };

} // end anonymous namespace

TEST_F (IndirectStringAdder, NothingAdded) {
    mock_mutex mutex;
    auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});
    auto const name_index = pstore::index::get_name_index (db_);

    auto adder = pstore::make_indirect_string_adder (transaction, name_index);
    adder.flush ();
    EXPECT_EQ (transaction.size (), 0U);
    transaction.commit ();
}

TEST_F (IndirectStringAdder, NewString) {
    constexpr auto str = "string";
    {
        mock_mutex mutex;
        auto transaction = begin (db_, std::unique_lock<mock_mutex>{mutex});
        {
            auto const name_index = pstore::index::get_name_index (db_);

            // Use the string adder to insert a string into the index and flush it to the store.
            auto adder = pstore::make_indirect_string_adder (transaction, name_index);
            pstore::shared_sstring_view const sstring1 = make_shared_sstring_view (str);
            pstore::shared_sstring_view const sstring2 = make_shared_sstring_view (str);
            {
                std::pair<pstore::index::name_index::iterator, bool> res1 = adder.add (&sstring1);
                EXPECT_EQ (res1.first->as_string_view (), sstring1);
                EXPECT_TRUE (res1.second);
            }
            {
                // adding the same string again should result in nothing being written.
                std::pair<pstore::index::name_index::iterator, bool> res2 = adder.add (&sstring2);
                EXPECT_EQ (res2.first->as_string_view (), sstring1);
                EXPECT_FALSE (res2.second);
            }
            EXPECT_EQ (transaction.size (), sizeof (pstore::address));
            adder.flush ();
        }
        transaction.commit ();
    }
    {
        auto const name_index = pstore::index::get_name_index (db_);
        pstore::shared_sstring_view const sstring = make_shared_sstring_view (str);
        auto pos = name_index->find (pstore::indirect_string{db_, &sstring});
        ASSERT_NE (pos, name_index->cend ());
        EXPECT_EQ (pos->as_string_view (), make_shared_sstring_view (str));
    }
}

// eof: unittests/pstore/test_indirect_string.cpp
