//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//===- tools/import/import.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include <bitset>

#include "pstore/command_line/command_line.hpp"
#include "pstore/command_line/revision_opt.hpp"
#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/core/database.hpp"
#include "pstore/exchange/import_root.hpp"

using namespace pstore::command_line;
using namespace std::string_literals;

namespace {

    opt<std::string> db_path (positional, usage ("repository"),
                              desc ("Path of the pstore repository to be created."), required);
    opt<std::string> json_source (positional, usage ("[input]"),
                                  desc ("The export file to be read (stdin if not specified)."));

    bool is_file_input () { return json_source.get_num_occurrences () > 0; }

    FILE * open_input () {
        return is_file_input () ? std::fopen (json_source.get ().c_str (), "r") : stdin;
    }

    std::string input_name () { return is_file_input () ? json_source.get () : "stdin"s; }

} // end anonymous namespace

#ifdef _WIN32
int _tmain (int argc, TCHAR const * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        parse_command_line_options (argc, argv, "pstore import utility\n");

        if (pstore::file::exists (db_path.get ())) {
            error_stream << NATIVE_TEXT ("error: the import database must not be an existing file.")
                         << std::endl;
            return EXIT_FAILURE;
        }

        pstore::database db{db_path.get (), pstore::database::access_mode::writable};

        auto const close = [] (FILE * const file) {
            if (file != stdin) {
                std::fclose (file);
            }
        };
        std::unique_ptr<FILE, decltype (close)> infile{open_input (), close};
        if (infile.get () == nullptr) {
            auto const err = errno;
            error_stream << NATIVE_TEXT (R"(error: could not open ")")
                         << pstore::utf::to_native_string (input_name ()) << R"(": )"
                         << std::strerror (err) << std::endl;
            return EXIT_FAILURE;
        }

        auto parser = pstore::exchange::import::create_parser (db);

        std::vector<std::uint8_t> buffer;
        buffer.resize (65535);

        for (;;) {
            auto * const ptr = reinterpret_cast<char *> (buffer.data ());
            std::size_t const nread =
                std::fread (ptr, sizeof (std::uint8_t), buffer.size (), infile.get ());
            if (nread < buffer.size ()) {
                if (std::ferror (infile.get ())) {
                    error_stream << NATIVE_TEXT ("error: there was an error reading input")
                                 << std::endl;
                    exit_code = EXIT_FAILURE;
                    break;
                }
            }

            parser.input (ptr, ptr + nread);
            if (parser.has_error ()) {
                auto const coord = parser.coordinate ();
                error_stream << pstore::utf::to_native_string (input_name ()) << NATIVE_TEXT (":")
                             << coord.row << NATIVE_TEXT (":") << coord.column
                             << NATIVE_TEXT (": error: ")
                             << pstore::utf::to_native_string (parser.last_error ().message ())
                             << std::endl;
                exit_code = EXIT_FAILURE;
                break;
            }

            // Stop if we've reached the end of the file.
            if (std::feof (infile.get ())) {
                parser.eof ();
                break;
            }
        }
    }
    // clang-format off
    PSTORE_CATCH (std::exception const & ex, { // clang-format on
        error_stream << NATIVE_TEXT ("error: ") << pstore::utf::to_native_string (ex.what ())
                     << std::endl;
        exit_code = EXIT_FAILURE;
    })
    // clang-format off
    PSTORE_CATCH (..., { // clang-format on
        error_stream << NATIVE_TEXT ("error: an unknown error occurred") << std::endl;
        exit_code = EXIT_FAILURE;
    })
    return exit_code;
}
