//===- tools/read/main.cpp ------------------------------------------------===//
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

#include <fstream>
#include <iostream>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    ifdef _MSC_VER
#        include <fcntl.h>
#        include <io.h>
#    endif
#endif

#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#include "switches.hpp"

using pstore::command_line::error_stream;
using pstore::command_line::out_stream;

namespace {

    void set_output_stream_to_binary (pstore::gsl::not_null<FILE *> const file) {
#ifdef _MSC_VER
        // In Visual Studio, stream I/O routines operate on a file that is open in text mode and
        // where line-feed characters are translated into CR-LF combinations on output. Here,
        // the stream is placed in binary mode, in which the translation is suppressed.

        if (_setmode (_fileno (file), O_BINARY) == -1) {
            throw std::runtime_error ("Cannot set stream to binary mode");
        }
#else
        (void) file; // Avoid an unused argument warning.
#endif
    }

    bool read_strings_index (pstore::database const & db, std::string const & key) {
        std::shared_ptr<pstore::index::name_index const> const strings =
            pstore::index::get_index<pstore::trailer::indices::name> (db);
        if (strings == nullptr) {
            error_stream << PSTORE_NATIVE_TEXT ("Error: Strings index was not found") << std::endl;
            return false;
        }

        auto str = pstore::make_sstring_view (key);
        auto const it = strings->find (db, pstore::indirect_string{db, &str});
        if (it == strings->cend (db)) {
            error_stream << pstore::utf::to_native_string (key)
                         << PSTORE_NATIVE_TEXT (": not found") << std::endl;
            // note that the program does not signal failure if the key is missing.
        } else {
            pstore::shared_sstring_view owner;
            out_stream << pstore::utf::to_native_string (
                it->as_db_string_view (&owner).to_string ());
        }
        return true;
    }


    bool read_names_index (pstore::database const & db, std::string const & key) {
        std::shared_ptr<pstore::index::write_index const> const names =
            pstore::index::get_index<pstore::trailer::indices::write> (db);
        if (names == nullptr) {
            error_stream << PSTORE_NATIVE_TEXT ("Error: Names index was not found") << std::endl;
            return false;
        }

        auto const it = names->find (db, key);
        if (it == names->cend (db)) {
            error_stream << pstore::utf::to_native_string (key)
                         << PSTORE_NATIVE_TEXT (": not found") << std::endl;
            // note that the program does not signal failure if the key is missing.
        } else {
            pstore::extent<char> const & r = it->second;
            std::shared_ptr<char const> ptr = db.getro (r);

            std::ostream & out = std::cout;
            out.exceptions (std::ostream::failbit | std::ostream::badbit);

            std::uint64_t size = r.size;
            constexpr auto stream_size_max = std::numeric_limits<std::streamsize>::max ();
            static_assert (stream_size_max > 0, "streamsize must be able to hold positive values");
            set_output_stream_to_binary (stdout);

            while (size > 0) {
                std::streamsize const size_to_write =
                    size > stream_size_max ? stream_size_max : static_cast<std::streamsize> (size);
                PSTORE_ASSERT (size_to_write > 0);
                out.write (reinterpret_cast<char const *> (ptr.get ()), size_to_write);
                size -= static_cast<
                    std::make_unsigned<std::remove_const<decltype (size_to_write)>::type>::type> (
                    size_to_write);
            }
        }

        return true;
    }

} // end anonymous namespace

#if defined(_WIN32)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;

    PSTORE_TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        pstore::database db{opt.db_path, pstore::database::access_mode::read_only};
        db.sync (opt.revision);

        bool const ok =
            opt.string_mode ? read_strings_index (db, opt.key) : read_names_index (db, opt.key);
        exit_code = ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        error_stream << PSTORE_NATIVE_TEXT ("Error: ") << pstore::utf::to_native_string (ex.what ())
                     << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        error_stream << PSTORE_NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    })

    return exit_code;
}
