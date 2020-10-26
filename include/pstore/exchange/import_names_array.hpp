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
//===- include/pstore/exchange/import_names_array.hpp ---------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP
#define PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP

#include "pstore/core/transaction.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_rule.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            template <typename TransactionLock>
            class names_array_members final : public rule {
            public:
                names_array_members (not_null<context *> ctxt,
                                     not_null<transaction<TransactionLock> *> transaction,
                                     not_null<name_mapping *> names);
                names_array_members (names_array_members const &) = delete;
                names_array_members (names_array_members &&) noexcept = delete;

                ~names_array_members () noexcept override = default;

                names_array_members & operator= (names_array_members const &) = delete;
                names_array_members & operator= (names_array_members &&) noexcept = delete;

            private:
                std::error_code string_value (std::string const & str) override;
                std::error_code end_array () override;
                gsl::czstring name () const noexcept override;

                not_null<transaction<TransactionLock> *> const transaction_;
                not_null<name_mapping *> const names_;
            };

            // (ctor)
            // ~~~~~~
            template <typename TransactionLock>
            names_array_members<TransactionLock>::names_array_members (
                not_null<context *> const ctxt,
                not_null<transaction<TransactionLock> *> const transaction,
                not_null<name_mapping *> const names)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names} {}

            // string value
            // ~~~~~~~~~~~~
            template <typename TransactionLock>
            std::error_code
            names_array_members<TransactionLock>::string_value (std::string const & str) {
                return names_->add_string (transaction_.get (), str);
            }

            // end array
            // ~~~~~~~~~
            template <typename TransactionLock>
            std::error_code names_array_members<TransactionLock>::end_array () {
                names_->flush (transaction_);
                return pop ();
            }

            // name
            // ~~~~
            template <typename TransactionLock>
            gsl::czstring names_array_members<TransactionLock>::name () const noexcept {
                return "names array members";
            }

        } // end namespace import
    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_NAMES_ARRAY_HPP
