//===- tools/diff/main.cpp ------------------------------------------------===//
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
/// \file main.cpp

#include <iostream>

#include "pstore/command_line/tchar.hpp"
#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#include "./switches.hpp"

using pstore::command_line::error_stream;
using pstore::command_line::out_stream;

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

        if (opt.hex) {
            pstore::dump::number_base::hex ();
        } else {
            pstore::dump::number_base::dec ();
        }

        pstore::database db (opt.db_path, pstore::database::access_mode::read_only);
        std::tie (opt.first_revision, opt.second_revision) = pstore::diff_dump::update_revisions (
            std::make_pair (opt.first_revision, opt.second_revision), db.get_current_revision ());

        pstore::dump::object::container file;
        file.emplace_back ("indices", pstore::diff_dump::make_indices_diff (db, opt.first_revision,
                                                                       *opt.second_revision));

        auto output = pstore::dump::make_value (file);
        out_stream << NATIVE_TEXT ("---\n") << *output << NATIVE_TEXT ("\n...\n");
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
