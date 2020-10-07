//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//===- tools/import/import.cpp --------------------------------------------===//
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
#include <bitset>

#include "pstore/cmd_util/command_line.hpp"
#include "pstore/cmd_util/revision_opt.hpp"
#include "pstore/cmd_util/str_to_revision.hpp"
#include "pstore/core/database.hpp"
#include "pstore/exchange/import_root.hpp"

using namespace pstore::cmd_util;

namespace {

    // TODO: cl::desc is used as the short-form list of arguments. We're providing full help text!
    cl::opt<std::string> db_path (cl::positional, cl::usage ("repository"),
                                  cl::desc ("Path of the pstore repository to be created."),
                                  cl::required);
    cl::opt<std::string>
        json_source (cl::positional, cl::usage ("[input]"),
                     cl::desc ("The export file to be read (stdin if not specified)."));

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR const * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        cl::parse_command_line_options (argc, argv, "pstore import utility\n");

        if (pstore::file::exists (db_path.get ())) {
            std::cerr << "The import database must not be an existing file.\n";
            return EXIT_FAILURE;
        }

        // TODO: check that db_path does not exist.
        pstore::database db{db_path.get (), pstore::database::access_mode::writable};
        FILE * infile = json_source.get_num_occurrences () > 0
                            ? std::fopen (json_source.get ().c_str (), "r")
                            : stdin;
        if (infile == nullptr) {
            std::perror ("File opening failed");
            return EXIT_FAILURE;
        }

        auto parser = pstore::exchange::create_import_parser (db);

        std::vector<std::uint8_t> buffer;
        buffer.resize (65535);

        for (;;) {
            auto * const ptr = reinterpret_cast<char *> (buffer.data ());
            std::size_t const nread =
                std::fread (ptr, sizeof (std::uint8_t), buffer.size (), infile);
            if (nread < buffer.size ()) {
                if (std::ferror (infile)) {
                    std::cerr << "There was an error reading input\n";
                    exit_code = EXIT_FAILURE;
                    break;
                }
            }

            parser.input (ptr, ptr + nread);
            if (parser.has_error ()) {
                std::error_code const erc = parser.last_error ();
                std::cerr << "Error: " << erc.message () << '\n'
                          << "Line " << parser.coordinate ().row << ", column "
                          << parser.coordinate ().column << '\n';
                exit_code = EXIT_FAILURE;
                break;
            }

            if (std::feof (infile)) {
                parser.eof ();
                break;
            }
        }

        if (infile != nullptr && infile != stdin) {
            std::fclose (infile);
            infile = nullptr;
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        std::cerr << "Error: " << ex.what () << '\n';
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        std::cerr << "Error: an unknown error occurred\n";
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
