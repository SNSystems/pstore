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

#include "empty_store.hpp"
#include "mock_mutex.hpp"
#include "pstore/serialize/types.hpp"

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
    auto const view = make_shared_sstring_view ("hello");
    pstore::indirect_string const x (db_, &view);
    pstore::indirect_string const y (db_, &view);
    EXPECT_TRUE (operator== (x, y));
    EXPECT_FALSE (operator!= (x, y));
}


namespace {

    pstore::address write_indirect_string (pstore::database & db,
                                           pstore::indirect_string const & sa) {
        using namespace pstore;

        mock_mutex mutex;
        auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
        auto const result = serialize::write (serialize::archive::make_writer (transaction), sa);
        transaction.commit ();
        return result;
    }

} // end anonymous namespace

TEST_F (IndirectString, WriteStoreRefToMemory) {
    auto str = make_shared_sstring_view ("hello");
    auto position = write_indirect_string (db_, pstore::indirect_string (db_, &str));

    auto const actual = db_.getro<pstore::address> (position);
    EXPECT_EQ (actual->absolute (), reinterpret_cast<std::uintptr_t> (&str) | 0x01);
}


namespace {

    struct string_address_pair {
        string_address_pair (pstore::address b, pstore::address i)
                : body{b}
                , indirect{i} {}
        pstore::address const body;
        pstore::address const indirect;
    };

    // Writes a string view followed by the address of the string view.
    string_address_pair write_indirect_string_view (pstore::database & db,
                                                    pstore::shared_sstring_view const & str) {
        using namespace pstore;

        mock_mutex mutex;
        auto transaction = begin (db, std::unique_lock<mock_mutex>{mutex});
        auto writer = serialize::archive::make_writer (transaction);
        address const string_body_pos = serialize::write (writer, str);
        address const string_address_pos = serialize::write (writer, string_body_pos);
        transaction.commit ();
        return {string_body_pos, string_address_pos};
    }

} // end anonymous namespace

TEST_F (IndirectString, ReadFromStore) {
    string_address_pair const sa =
        write_indirect_string_view (db_, make_shared_sstring_view ("hello"));

    using namespace pstore::serialize;
    auto const result = read<pstore::indirect_string> (archive::make_reader (db_, sa.indirect));
    EXPECT_FALSE (result.is_pointer_);
    EXPECT_EQ (result.address_, sa.body.absolute ());
}

// FIXME. FIXME.
#if 0
TEST_F (NewName, EqualityBetweenInMemoryAndInStore) {
    auto const view = make_shared_sstring_view ("string");
    string_address_pair const sa = write_indirect_string_view(db_, view);
    pstore::indirect_string const x (db_, &view);
    pstore::indirect_string const y (db_, sa.indirect);
    EXPECT_TRUE (operator== (x, y));
    EXPECT_FALSE (operator!= (x, y));
}

TEST_F (NewName, EqualityBetweenInStoreAndInStore) {
    auto const view = make_shared_sstring_view ("string");
    string_address_pair const s1 = write_indirect_string_view (db_, view);
    string_address_pair const s2 = write_indirect_string_view (db_, view);
    pstore::indirect_string const x (db_, s1.indirect);
    pstore::indirect_string const y (db_, s2.indirect);
    EXPECT_TRUE (operator== (x, y));
    EXPECT_FALSE (operator!= (x, y));
}
#endif

TEST_F (IndirectString, EqualityBetweenInMemoryAndInMemory) {
    auto src = "string";
    auto const view1 = make_shared_sstring_view (src);
    auto const view2 = make_shared_sstring_view (src);
    pstore::indirect_string const x (db_, &view1);
    pstore::indirect_string const y (db_, &view2);
    EXPECT_TRUE (operator== (x, y));
    EXPECT_FALSE (operator!= (x, y));
}
// eof: unittests/pstore/test_indirect_string.cpp
