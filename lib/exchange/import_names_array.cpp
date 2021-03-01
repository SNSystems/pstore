//===- lib/exchange/import_names_array.cpp --------------------------------===//
//*  _                            _                                      *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   __ _ _ __ ___   ___  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _` | '_ ` _ \ / _ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_| | | | | | |  __/\__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\__,_|_| |_| |_|\___||___/ *
//*             |_|                                                      *
//*                              *
//*   __ _ _ __ _ __ __ _ _   _  *
//*  / _` | '__| '__/ _` | | | | *
//* | (_| | |  | | | (_| | |_| | *
//*  \__,_|_|  |_|  \__,_|\__, | *
//*                       |___/  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_names_array.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            // (ctor)
            // ~~~~~~
            names_array_members::names_array_members (
                not_null<context *> const ctxt, not_null<transaction_base *> const transaction,
                not_null<name_mapping *> const names)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names} {}

            // string value
            // ~~~~~~~~~~~~
            std::error_code names_array_members::string_value (std::string const & str) {
                return names_->add_string (transaction_.get (), str);
            }

            // end array
            // ~~~~~~~~~
            std::error_code names_array_members::end_array () {
                names_->flush (transaction_);
                return pop ();
            }

            // name
            // ~~~~
            gsl::czstring names_array_members::name () const noexcept {
                return "names array members";
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
