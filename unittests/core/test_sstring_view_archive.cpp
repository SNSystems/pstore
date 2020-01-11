//*          _        _                     _                *
//*  ___ ___| |_ _ __(_)_ __   __ _  __   _(_) _____      __ *
//* / __/ __| __| '__| | '_ \ / _` | \ \ / / |/ _ \ \ /\ / / *
//* \__ \__ \ |_| |  | | | | | (_| |  \ V /| |  __/\ V  V /  *
//* |___/___/\__|_|  |_|_| |_|\__, |   \_/ |_|\___| \_/\_/   *
//*                           |___/                          *
//*                 _     _            *
//*   __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                    *
//===- unittests/core/test_sstring_view_archive.cpp -----------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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

#include "pstore/core/sstring_view_archive.hpp"
#include <cassert>
#include <vector>
#include <gmock/gmock.h>

#include "pstore/core/transaction.hpp"
#include "pstore/core/db_archive.hpp"

#include "empty_store.hpp"
#include "mock_mutex.hpp"

namespace {
    using shared_sstring_view = pstore::sstring_view<std::shared_ptr<char const>>;

    class SStringViewArchive : public EmptyStore {
    public:
        SStringViewArchive ()
                : db_{this->file ()} {
            db_.set_vacuum_mode (pstore::database::vacuum_mode::disabled);
        }

    protected:
        pstore::database db_;

        pstore::address current_pos (pstore::transaction_base & t) const;
        std::vector<char> as_vector (pstore::typed_address<char> first,
                                     pstore::typed_address<char> last) const;
        static shared_sstring_view make_shared_sstring_view (char const * s);
    };

    shared_sstring_view SStringViewArchive::make_shared_sstring_view (char const * s) {
        auto const length = std::strlen (s);
        auto ptr = std::shared_ptr<char> (new char[length], [](char * p) { delete[] p; });
        std::copy (s, s + length, ptr.get ());
        return {ptr, length};
    }

    pstore::address SStringViewArchive::current_pos (pstore::transaction_base & t) const {
        return t.allocate (0U, 1U); // allocate 0 bytes to get the current EOF.
    }

    std::vector<char> SStringViewArchive::as_vector (pstore::typed_address<char> first,
                                                     pstore::typed_address<char> last) const {
        assert (last >= first);
        if (last < first) {
            return {};
        }
        // Get the chars within the specified address range.
        std::size_t const num_chars = last.absolute () - first.absolute ();
        auto ptr = db_.getro (first, num_chars);
        // Convert them to a vector so that they're easy to compare.
        return {ptr.get (), ptr.get () + num_chars};
    }
} // namespace

TEST_F (SStringViewArchive, Empty) {
    auto str = make_shared_sstring_view ("");

    // Append 'str'' to the store (we don't need to have committed the transaction to be able to
    // access its contents).
    mock_mutex mutex;
    auto transaction = pstore::begin (db_, std::unique_lock<mock_mutex>{mutex});
    auto const first = pstore::typed_address<char> (this->current_pos (transaction));
    pstore::serialize::write (pstore::serialize::archive::make_writer (transaction), str);

    auto const last = pstore::typed_address<char> (this->current_pos (transaction));
    EXPECT_THAT (as_vector (first, last), ::testing::ElementsAre ('\x1', '\x0'));

    // Now try reading it back and compare to the original string.
    {
        using namespace pstore::serialize;
        shared_sstring_view const actual =
            read<shared_sstring_view> (archive::database_reader{db_, first.to_address ()});
        EXPECT_EQ (actual, "");
    }
}

TEST_F (SStringViewArchive, WriteHello) {
    auto str = make_shared_sstring_view ("hello");

    mock_mutex mutex;
    auto transaction = pstore::begin (db_, std::unique_lock<mock_mutex>{mutex});
    auto const first = pstore::typed_address<char> (this->current_pos (transaction));
    {
        auto writer = pstore::serialize::archive::make_writer (transaction);
        pstore::serialize::write (writer, str);
    }
    auto const last = pstore::typed_address<char> (this->current_pos (transaction));
    EXPECT_THAT (as_vector (first, last),
                 ::testing::ElementsAre ('\xb', '\x0', 'h', 'e', 'l', 'l', 'o'));
    {
        auto reader = pstore::serialize::archive::database_reader{db_, first.to_address ()};
        shared_sstring_view const actual = pstore::serialize::read<shared_sstring_view> (reader);
        EXPECT_EQ (actual, "hello");
    }
}
