//===- lib/command_line/revision_opt.cpp ----------------------------------===//
//*                 _     _                           _    *
//*  _ __ _____   _(_)___(_) ___  _ __     ___  _ __ | |_  *
//* | '__/ _ \ \ / / / __| |/ _ \| '_ \   / _ \| '_ \| __| *
//* | | |  __/\ V /| \__ \ | (_) | | | | | (_) | |_) | |_  *
//* |_|  \___| \_/ |_|___/_|\___/|_| |_|  \___/| .__/ \__| *
//*                                            |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/command_line/revision_opt.hpp"

#include <cstdlib>
#include <iostream>

#include "pstore/command_line/str_to_revision.hpp"
#include "pstore/command_line/tchar.hpp"

namespace pstore {
    namespace command_line {

        revision_opt & revision_opt::operator= (std::string const & val) {
            if (!val.empty ()) {
                auto rp = pstore::str_to_revision (val);
                if (!rp) {
                    error_stream << PSTORE_NATIVE_TEXT (
                        "Error: revision must be a revision number or 'HEAD'\n");
                    std::exit (EXIT_FAILURE);
                }
                r_ = *rp;
            }
            return *this;
        }

        gsl::czstring type_description<revision_opt>::value = "rev";

    } // end namespace command_line
} // end namespace pstore
