//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/read/main.cpp ------------------------------------------------===//
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

#include <fstream>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#endif
#else
// On Windows, the TCHAR type may be either char or whar_t depending on the selected
// Unicode mode. Everywhere else, I need to add this type for compatibility.
using TCHAR = char;
#endif

#include "switches.hpp"

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/sstring_view_archive.hpp"
#include "pstore/cmd_util/str_to_revision.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

namespace {
#if defined(_WIN32) && defined(_UNICODE)
    auto & error_stream = std::wcerr;
    auto & out_stream = std::wcout;
#else
    auto & error_stream = std::cerr;
    auto & out_stream = std::cout;
#endif

    void set_output_stream_to_binary (FILE * const fd) {
        if (fd) {
#ifdef _MSC_VER
            // On Visual Studio, a stream I/O routine operates on a file that is open in text mode.
            // Line feed characters are translated into CR-LF combinations on output. The text mode
            // could be changed to binary mode, in which the translation is suppressed.
            int result = _setmode (_fileno (stdout), O_BINARY);
            if (result == -1) {
                throw std::runtime_error ("Cannot set stream to binary mode");
            }
#endif
        }
    }


    bool read_strings_index (pstore::database & db, std::string const & key) {
        bool ok = true;

        std::shared_ptr<pstore::index::name_index> const strings =
            pstore::index::get_name_index (db);
        if (strings == nullptr) {
            error_stream << NATIVE_TEXT ("Error: Strings index was not found\n");
            ok = false;
        } else {
            auto str = pstore::make_sstring_view (key);
            auto const it = strings->find (pstore::indirect_string{db, &str});
            if (it == strings->cend ()) {
                error_stream << pstore::utf::to_native_string (key) << NATIVE_TEXT (": not found")
                             << std::endl;
                // note that the program does not signal failure if the key is missing.
            } else {
                out_stream << pstore::utf::to_native_string (it->as_string_view ().to_string ());
            }
        }
        return ok;
    }


    bool read_names_index (pstore::database & db, std::string const & key) {
        bool ok = true;

        std::shared_ptr<pstore::index::write_index> const names =
            pstore::index::get_write_index (db);
        if (names == nullptr) {
            error_stream << NATIVE_TEXT ("Error: Names index was not found\n");
            ok = false;
        } else {
            auto const it = names->find (key);
            if (it == names->cend ()) {
                error_stream << pstore::utf::to_native_string (key) << NATIVE_TEXT (": not found")
                             << std::endl;
                // note that the program does not signal failure if the key is missing.
            } else {
                pstore::extent const & r = it->second;
                std::shared_ptr<void const> ptr = db.getro (r);

                std::ostream & out = std::cout;
                out.exceptions (std::ofstream::failbit | std::ofstream::badbit);

                std::uint64_t size = r.size;
                constexpr auto const stream_size_max = std::numeric_limits<std::streamsize>::max ();
                static_assert (stream_size_max > 0,
                               "streamsize must be able to hold positive values");
                set_output_stream_to_binary (stdout);

                while (size > 0) {
                    std::streamsize const size_to_write = size > stream_size_max
                                                              ? stream_size_max
                                                              : static_cast<std::streamsize> (size);
                    assert (size_to_write > 0);
                    out.write (reinterpret_cast<char const *> (ptr.get ()), size_to_write);
                    size -= static_cast<std::make_unsigned<decltype (size_to_write)>::type> (
                        size_to_write);
                }
            }
        }
        return ok;
    }
} // namespace

#if defined(_WIN32) && !defined(PSTORE_IS_INSIDE_LLVM)
int _tmain (int argc, TCHAR * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    using pstore::utf::from_native_string;
    using pstore::utf::to_native_string;

    PSTORE_TRY {
        switches opt;
        std::tie (opt, exit_code) = get_switches (argc, argv);
        if (exit_code != EXIT_SUCCESS) {
            return exit_code;
        }

        pstore::database db (opt.db_path, pstore::database::access_mode::read_only);
        db.sync (opt.revision);

        bool const string_mode = opt.string_mode;
        std::string const key = opt.key;
        bool ok = true;
        if (string_mode) {
            ok = read_strings_index (db, key);
        } else {
            ok = read_names_index (db, key);
        }
        if (!ok) {
            exit_code = EXIT_FAILURE;
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, {
        error_stream << NATIVE_TEXT ("Error: ") << pstore::utf::to_native_string (ex.what ())
                    << std::endl;
        exit_code = EXIT_FAILURE;
    })
    PSTORE_CATCH (..., {
        error_stream << NATIVE_TEXT ("Unknown error.") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format on

    return exit_code;
}

// eof: tools/read/main.cpp
