//*                             _    *
//*   _____  ___ __   ___  _ __| |_  *
//*  / _ \ \/ / '_ \ / _ \| '__| __| *
//* |  __/>  <| |_) | (_) | |  | |_  *
//*  \___/_/\_\ .__/ \___/|_|   \__| *
//*           |_|                    *
//===- tools/export/export.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "pstore/exchange/export.hpp"
#include "pstore/core/database.hpp"
#include "pstore/command_line/command_line.hpp"

using namespace pstore::command_line;

namespace {

    opt<std::string> db_path (positional, usage ("repository"),
                              desc ("Path of the pstore repository to be exported."), required);

} // end anonymous namespace.

#ifdef _WIN32
int _tmain (int argc, TCHAR const * argv[]) {
#else
int main (int argc, char * argv[]) {
#endif
    int exit_code = EXIT_SUCCESS;
    PSTORE_TRY {
        parse_command_line_options (argc, argv, "pstore export utility\n");

        pstore::exchange::export_ns::ostream os{stdout};
        pstore::database db{db_path.get (), pstore::database::access_mode::read_only};
        pstore::exchange::export_ns::emit_database (db, os, true);
        os.flush ();
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
