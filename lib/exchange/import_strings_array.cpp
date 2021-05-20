//===- lib/exchange/import_strings_array.cpp ------------------------------===//
//*  _                            _         _        _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_   ___| |_ _ __(_)_ __   __ _ ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| / __| __| '__| | '_ \ / _` / __| *
//* | | | | | | | |_) | (_) | |  | |_  \__ \ |_| |  | | | | | (_| \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |___/\__|_|  |_|_| |_|\__, |___/ *
//*             |_|                                          |___/      *
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
#include "pstore/exchange/import_strings_array.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            // (ctor)
            // ~~~~~~
            strings_array_members::strings_array_members (
                not_null<context *> const ctxt, not_null<transaction_base *> const transaction,
                not_null<string_mapping *> const strings)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , strings_{strings} {}

            // string value
            // ~~~~~~~~~~~~
            std::error_code strings_array_members::string_value (std::string const & str) {
                return strings_->add_string (transaction_.get (), str);
            }

            // end array
            // ~~~~~~~~~
            std::error_code strings_array_members::end_array () {
                strings_->flush (transaction_);
                return pop ();
            }

            // name
            // ~~~~
            gsl::czstring strings_array_members::name () const noexcept {
                return "strings array members";
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore
