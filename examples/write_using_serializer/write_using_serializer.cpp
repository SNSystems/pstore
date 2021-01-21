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
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

            auto index = pstore::index::get_index<pstore::trailer::indices::write> (db);
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
