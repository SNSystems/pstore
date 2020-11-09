//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*  _                                  _   _              *
//* | |_ _ __ __ _ _ __  ___  __ _  ___| |_(_) ___  _ __   *
//* | __| '__/ _` | '_ \/ __|/ _` |/ __| __| |/ _ \| '_ \  *
//* | |_| | | (_| | | | \__ \ (_| | (__| |_| | (_) | | | | *
//*  \__|_|  \__,_|_| |_|___/\__,_|\___|\__|_|\___/|_| |_| *
//*                                                        *
//===- include/pstore/exchange/import_transaction.hpp ---------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_TRANSACTION_HPP
#define PSTORE_EXCHANGE_IMPORT_TRANSACTION_HPP

#include "pstore/core/transaction.hpp"
#include "pstore/exchange/import_compilation.hpp"
#include "pstore/exchange/import_debug_line_header.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_fragment.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_names_array.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_rule.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            template <typename TransactionLock>
            class transaction_contents final : public rule {
            public:
                transaction_contents (not_null<context *> const ctxt,
                                      not_null<name_mapping *> const names)
                        : rule (ctxt)
                        , transaction_{begin (*ctxt->db)}
                        , names_{names} {}
                transaction_contents (transaction_contents const &) = delete;
                transaction_contents (transaction_contents &&) noexcept = delete;

                ~transaction_contents () noexcept override = default;

                transaction_contents & operator= (transaction_contents const &) = delete;
                transaction_contents & operator= (transaction_contents &&) noexcept = delete;

            private:
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;
                gsl::czstring name () const noexcept override;

                std::error_code apply_patches ();

                transaction<TransactionLock> transaction_;
                not_null<name_mapping *> names_;
            };

            // name
            // ~~~~
            template <typename TransactionLock>
            gsl::czstring transaction_contents<TransactionLock>::name () const noexcept {
                return "transaction contents";
            }

            // key
            // ~~~
            template <typename TransactionLock>
            std::error_code transaction_contents<TransactionLock>::key (std::string const & s) {
                // TODO: check that "names" is the first key that we see.
                if (s == "names") {
                    return push_array_rule<names_array_members> (this, &transaction_, names_);
                }
                if (s == "debugline") {
                    return push_object_rule<debug_line_index> (this, &transaction_);
                }
                if (s == "fragments") {
                    return push_object_rule<fragment_index> (this, &transaction_, names_.get ());
                }
                if (s == "compilations") {
                    return push_object_rule<compilations_index> (this, &transaction_,
                                                                 names_.get ());
                }
                return error::unknown_transaction_object_key;
            }

            // end object
            // ~~~~~~~~~~
            template <typename TransactionLock>
            std::error_code transaction_contents<TransactionLock>::end_object () {
                if (auto const erc = this->apply_patches ()) {
                    return erc;
                }
                transaction_.commit ();
                return pop ();
            }

            // apply patches
            // ~~~~~~~~~~~~~
            template <typename TransactionLock>
            std::error_code transaction_contents<TransactionLock>::apply_patches () {
                return this->get_context ()->apply_patches (&transaction_);
            }


            template <typename TransactionLock>
            class transaction_object final : public rule {
            public:
                transaction_object (not_null<context *> const ctxt,
                                    not_null<name_mapping *> const names)
                        : rule (ctxt)
                        , names_{names} {}
                transaction_object (transaction_object const &) = delete;
                transaction_object (transaction_object &&) noexcept = delete;

                ~transaction_object () noexcept override = default;

                transaction_object & operator= (transaction_object const &) = delete;
                transaction_object & operator= (transaction_object &&) noexcept = delete;

                pstore::gsl::czstring name () const noexcept override {
                    return "transaction object";
                }
                std::error_code begin_object () override {
                    return push<transaction_contents<TransactionLock>> (names_);
                }
                std::error_code end_array () override { return pop (); }

            private:
                not_null<name_mapping *> const names_;
            };

            //*  _                             _   _                                    *
            //* | |_ _ _ __ _ _ _  ___ __ _ __| |_(_)___ _ _    __ _ _ _ _ _ __ _ _  _  *
            //* |  _| '_/ _` | ' \(_-</ _` / _|  _| / _ \ ' \  / _` | '_| '_/ _` | || | *
            //*  \__|_| \__,_|_||_/__/\__,_\__|\__|_\___/_||_| \__,_|_| |_| \__,_|\_, | *
            //*                                                                   |__/  *
            template <typename TransactionLock>
            class transaction_array final : public rule {
            public:
                transaction_array (not_null<context *> ctxt, not_null<name_mapping *> names);
                transaction_array (transaction_array const &) = delete;
                transaction_array (transaction_array &&) noexcept = delete;

                ~transaction_array () noexcept override = default;

                transaction_array & operator= (transaction_array const &) = delete;
                transaction_array & operator= (transaction_array &&) noexcept = delete;

                gsl::czstring name () const noexcept override;
                std::error_code begin_array () override;

            private:
                not_null<name_mapping *> const names_;
            };

            // (ctor)
            // ~~~~~~
            template <typename TransactionLock>
            transaction_array<TransactionLock>::transaction_array (
                not_null<context *> const ctxt, not_null<name_mapping *> const names)
                    : rule (ctxt)
                    , names_{names} {}

            // name
            // ~~~~
            template <typename TransactionLock>
            gsl::czstring transaction_array<TransactionLock>::name () const noexcept {
                return "transaction array";
            }

            // begin array
            // ~~~~~~~~~~~
            template <typename TransactionLock>
            std::error_code transaction_array<TransactionLock>::begin_array () {
                return this->replace_top<transaction_object<TransactionLock>> (names_);
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_TRANSACTION_HPP
