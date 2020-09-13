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
//===- include/pstore/exchange/import_debug_line_header.hpp ---------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
#define PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"
#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {

        template <typename TransactionLock>
        class debug_line_index final : public import_rule {
        public:
            using transaction_pointer = transaction<TransactionLock> *;

            debug_line_index (parse_stack_pointer const stack,
                              transaction_pointer const transaction);
            debug_line_index (debug_line_index const &) = delete;
            debug_line_index (debug_line_index &&) noexcept = delete;

            debug_line_index & operator= (debug_line_index const &) = delete;
            debug_line_index & operator= (debug_line_index &&) noexcept = delete;

            gsl::czstring name () const noexcept override;

            std::error_code string_value (std::string const & s) override;
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            std::shared_ptr<index::debug_line_header_index> index_;
            index::digest digest_;

            transaction_pointer transaction_;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        debug_line_index<TransactionLock>::debug_line_index (parse_stack_pointer const stack,
                                                             transaction_pointer const transaction)
                : import_rule (stack)
                , index_{index::get_index<trailer::indices::debug_line_header> (transaction->db ())}
                , transaction_{transaction} {}

        // name
        // ~~~~
        template <typename TransactionLock>
        gsl::czstring debug_line_index<TransactionLock>::name () const noexcept {
            return "debug line index";
        }

        // string value
        // ~~~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code debug_line_index<TransactionLock>::string_value (std::string const & s) {
            // Decode the received string to get the raw binary.
            std::vector<std::uint8_t> data;
            if (!from_base64 (std::begin (s), std::end (s), std::back_inserter (data))) {
                return import_error::bad_base64_data;
            }

            // Create space for this data in the store.
            std::shared_ptr<std::uint8_t> out;
            typed_address<std::uint8_t> where;
            std::tie (out, where) = transaction_->template alloc_rw<std::uint8_t> (data.size ());

            // Copy the data to the store.
            std::copy (std::begin (data), std::end (data), out.get ());

            // Add an index entry for this data.
            index_->insert (*transaction_,
                            std::make_pair (digest_, extent<std::uint8_t>{where, data.size ()}));
            return {};
        }

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code debug_line_index<TransactionLock>::key (std::string const & s) {
            if (maybe<uint128> const digest = digest_from_string (s)) {
                digest_ = *digest;
                return {};
            }
            return import_error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code debug_line_index<TransactionLock>::end_object () {
            return pop ();
        }


    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_HEADER_HPP
