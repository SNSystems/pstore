//*                _ _                   _              *
//* __      ___ __(_) |_ ___   _   _ ___(_)_ __   __ _  *
//* \ \ /\ / / '__| | __/ _ \ | | | / __| | '_ \ / _` | *
//*  \ V  V /| |  | | ||  __/ | |_| \__ \ | | | | (_| | *
//*   \_/\_/ |_|  |_|\__\___|  \__,_|___/_|_| |_|\__, | *
//*                                              |___/  *
//*                _       _ _               *
//*  ___  ___ _ __(_) __ _| (_)_______ _ __  *
//* / __|/ _ \ '__| |/ _` | | |_  / _ \ '__| *
//* \__ \  __/ |  | | (_| | | |/ /  __/ |    *
//* |___/\___|_|  |_|\__,_|_|_/___\___|_|    *
//*                                          *
//===- examples/write_using_serializer/write_using_serializer.cpp ---------===//
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

#include <cstdlib>
#include <exception>
#include <iostream>

#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/support/portab.hpp"

using namespace pstore;

int main () {
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        auto const key = std::string{"key"};
        auto const value = std::string{"hello world\n"};

        database db ("./write_using_serializer.db", pstore::database::access_mode::writable);
        auto t = begin (db); // Start a transaction

        {
            auto archive = serialize::archive::make_writer (t);
            auto const addr = typed_address<char> (serialize::write (archive, value));
            std::uint64_t const size = db.size () - addr.absolute ();

            auto index = pstore::index::get_write_index (db);
            index->insert_or_assign (t, key, make_extent (addr, size));
        }

        t.commit (); // Finalize the transaction.
    }
    PSTORE_CATCH (std::exception const & ex, {
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        std::cerr << "An unknown error occurred\n";
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
