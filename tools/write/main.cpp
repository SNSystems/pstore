//===- tools/write/main.cpp -----------------------------------------------===//
//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>

// pstore includes.
#include "pstore/core/db_archive.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/serialize/standard_types.hpp"
#include "pstore/support/error.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp" // for UTF-8 to UTF-16 conversion on Windows.

// Local includes
#include "switches.hpp"

using pstore::command_line::error_stream;

namespace {

    bool add_file (pstore::transaction_base & transaction, pstore::index::write_index & names,
                   std::string const & key, std::string const & path) {

        using namespace pstore::file;
        bool ok = true;
        pstore::file::file_handle file{path};
        file.open (pstore::file::file_handle::create_mode::open_existing,
                   pstore::file::file_handle::writable_mode::read_only);
        if (!file.is_open ()) {
            ok = false;
        } else {
            auto const size = file.size ();

            // Allocate space in the transaction for 'size' bytes.
            auto addr = pstore::typed_address<char>::null ();
            std::shared_ptr<char> ptr;
            std::tie (ptr, addr) = transaction.alloc_rw<char> (size);

            // Copy from the source file to the data store. The destination for the read_span() is
            // the memory that we just allocated in the data store.
            auto span = pstore::gsl::make_span (ptr.get (), static_cast<std::ptrdiff_t> (size));
            std::size_t const bytes_read = file.read_span (span);

            auto const expected_size = span.size_bytes ();
            PSTORE_ASSERT (expected_size >= 0);
            if (bytes_read !=
                static_cast<
                    std::make_unsigned<std::remove_const<decltype (expected_size)>::type>::type> (
                    expected_size)) {
                error_stream << PSTORE_NATIVE_TEXT ("Did not read the number of bytes requested");
                std::exit (EXIT_FAILURE);
            }

            // Add it to the names index.
            names.insert_or_assign (transaction, key, make_extent (addr, size));
        }

        return ok;
    }

    auto append_string (pstore::transaction_base & transaction, std::string const & v)
        -> pstore::extent<std::string::value_type> {
        // Since the read utility prefers to get raw string value in the system tests, this function
        // is changed to store raw string into the store instead of using serialize write.

        auto const size = v.size ();
        using element_type = typename std::string::value_type;

        // Allocate space in the transaction for the value block
        auto addr = pstore::typed_address<element_type>::null ();
        std::shared_ptr<element_type> ptr;
        std::tie (ptr, addr) = transaction.alloc_rw<element_type> (size);

        // Copy the string to the store.
        std::copy (std::begin (v), std::end (v), ptr.get ());

        return {addr, size};
    }

} // end anonymous namespace


#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    using pstore::utf::to_native_string;

    PSTORE_TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        pstore::database database (opt.db_path, pstore::database::access_mode::writable);
        database.set_vacuum_mode (opt.vmode);

        {
            // Start a transaction...
            auto transaction = pstore::begin (database);

            // Read the write and name indexes.
            std::shared_ptr<pstore::index::name_index> const name =
                pstore::index::get_index<pstore::trailer::indices::name> (database);
            std::shared_ptr<pstore::index::write_index> const write =
                pstore::index::get_index<pstore::trailer::indices::write> (database);

            // Scan through the string value arguments from the command line. These are of
            // the form key,value where value is a string which is stored directly.
            for (std::pair<std::string, std::string> const & v : opt.add) {
                write->insert_or_assign (transaction, v.first,
                                         append_string (transaction, v.second));
            }

            // Now record the files requested on the command line.
            for (std::pair<std::string, std::string> const & v : opt.files) {
                if (!add_file (transaction, *write, v.first, v.second)) {
                    error_stream << to_native_string (v.second)
                                 << PSTORE_NATIVE_TEXT (": No such file or directory\n");
                    exit_code = EXIT_FAILURE;
                }
            }

            // Scan through the string arguments from the command line.
            std::vector<pstore::raw_sstring_view> strings;
            strings.reserve (opt.strings.size ());
            pstore::indirect_string_adder adder;
            for (std::string const & str : opt.strings) {
                strings.emplace_back (pstore::make_sstring_view (str));
                auto & s = strings.back ();
                adder.add (transaction, name, &s);
            }
            adder.flush (transaction);

            transaction.commit ();
        }

        database.close ();
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        auto what = ex.what ();
        error_stream << PSTORE_NATIVE_TEXT ("An error occurred: ") << to_native_string (what)
                    << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        error_stream << PSTORE_NATIVE_TEXT ("An unknown error occurred.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}
