//===- lib/exchange/import_debug_line_header.cpp --------------------------===//
//*  _                            _         _      _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_    __| | ___| |__  _   _  __ _  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _` |/ _ \ '_ \| | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | (_| |  __/ |_) | |_| | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\___|_.__/ \__,_|\__, | *
//*             |_|                                            |___/  *
//*  _ _              _                    _            *
//* | (_)_ __   ___  | |__   ___  __ _  __| | ___ _ __  *
//* | | | '_ \ / _ \ | '_ \ / _ \/ _` |/ _` |/ _ \ '__| *
//* | | | | | |  __/ | | | |  __/ (_| | (_| |  __/ |    *
//* |_|_|_| |_|\___| |_| |_|\___|\__,_|\__,_|\___|_|    *
//*                                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_debug_line_header.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            // (ctor)
            // ~~~~~~
            debug_line_index::debug_line_index (not_null<context *> const ctxt,
                                                not_null<transaction_base *> const transaction)
                    : rule (ctxt)
                    , index_{index::get_index<trailer::indices::debug_line_header> (
                          transaction->db ())}
                    , transaction_{transaction} {}

            // name
            // ~~~~
            gsl::czstring debug_line_index::name () const noexcept { return "debug line index"; }

            // string value
            // ~~~~~~~~~~~~
            std::error_code debug_line_index::string_value (std::string const & s) {
                // Decode the received string to get the raw binary.
                std::vector<std::uint8_t> data;
                if (!from_base64 (std::begin (s), std::end (s), std::back_inserter (data))) {
                    return error::bad_base64_data;
                }

                // Create space for this data in the store.
                std::shared_ptr<std::uint8_t> out;
                typed_address<std::uint8_t> where;
                std::tie (out, where) =
                    transaction_->template alloc_rw<std::uint8_t> (data.size ());

                // Copy the data to the store.
                std::copy (std::begin (data), std::end (data), out.get ());

                // Add an index entry for this data.
                index_->insert (*transaction_, std::make_pair (digest_, extent<std::uint8_t>{
                                                                            where, data.size ()}));
                return {};
            }

            // key
            // ~~~
            std::error_code debug_line_index::key (std::string const & s) {
                if (maybe<uint128> const digest = uint128::from_hex_string (s)) {
                    digest_ = *digest;
                    return {};
                }
                return error::bad_digest;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code debug_line_index::end_object () { return pop (); }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
