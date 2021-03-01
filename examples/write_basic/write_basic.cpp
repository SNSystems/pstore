//===- examples/write_basic/write_basic.cpp -------------------------------===//
//*                _ _         _               _       *
//* __      ___ __(_) |_ ___  | |__   __ _ ___(_) ___  *
//* \ \ /\ / / '__| | __/ _ \ | '_ \ / _` / __| |/ __| *
//*  \ V  V /| |  | | ||  __/ | |_) | (_| \__ \ | (__  *
//*   \_/\_/ |_|  |_|\__\___| |_.__/ \__,_|___/_|\___| *
//*                                                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cstdlib>
#include <exception>
#include <iostream>

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/support/portab.hpp"

int main () {
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
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
