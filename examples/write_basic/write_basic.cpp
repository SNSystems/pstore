//*                _ _         _               _       *
//* __      ___ __(_) |_ ___  | |__   __ _ ___(_) ___  *
//* \ \ /\ / / '__| | __/ _ \ | '_ \ / _` / __| |/ __| *
//*  \ V  V /| |  | | ||  __/ | |_) | (_| \__ \ | (__  *
//*   \_/\_/ |_|  |_|\__\___| |_.__/ \__,_|___/_|\___| *
//*                                                    *
//===- examples/write_basic/write_basic.cpp -------------------------------===//
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
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"

int main () {
    auto const key = std::string{"key"};
    auto const value = std::string{"hello world\n"};

    pstore::database db ("./write_example.db", pstore::database::access_mode::writable);
    auto t = pstore::begin (db); // Start a transaction

    {
        auto const size = value.length ();

        // Allocate space for the value.
        pstore::typed_address<char> addr;
        std::shared_ptr<char> ptr;
        std::tie (ptr, addr) = t.alloc_rw<char> (size);

        std::memcpy (ptr.get (), value.data (), size); // Copy it to the store.

        // Tell the index about this new data.
        auto index = pstore::index::get_write_index (db);
        index->insert_or_assign (t, key, pstore::extent{addr.to_address (), size});
    }

    t.commit (); // Finalize the transaction.
}
// eof: examples/write_basic/write_basic.cpp
