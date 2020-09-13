//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*                            _ _       _   _              *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \  *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | | *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_| *
//*                     |_|                                 *
//===- include/pstore/exchange/import_compilation.hpp ---------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP
#define PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP

#include <bitset>

#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {

        using fragment_index_pointer = not_null<std::shared_ptr<index::fragment_index>>;

        //*     _      __ _      _ _   _           *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
        //*                                        *
        //-MARK: import definition
        class import_definition final : public import_rule {
        public:
            using container = std::vector<repo::compilation_member>;
            using container_pointer = not_null<container *>;
            using names_pointer = not_null<import_name_mapping const *>;

            import_definition (parse_stack_pointer const stack, container_pointer const definitions,
                               names_pointer const names, database const & db,
                               fragment_index_pointer const & fragments);
            import_definition (import_definition const &) = delete;
            import_definition (import_definition &&) noexcept = delete;
            ~import_definition () noexcept override = default;

            import_definition & operator= (import_definition const &) = delete;
            import_definition & operator= (import_definition &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "definition"; }

            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

            static maybe<repo::linkage> decode_linkage (std::string const & linkage);
            static maybe<repo::visibility> decode_visibility (std::string const & visibility);

        private:
            container_pointer const definitions_;
            names_pointer const names_;
            database const & db_;
            fragment_index_pointer const fragments_;

            std::string digest_;
            std::uint64_t name_ = 0;
            std::string linkage_;
            std::string visibility_;
        };


        //*     _      __ _      _ _   _                _     _        _    *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _    ___| |__ (_)___ __| |_  *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  / _ \ '_ \| / -_) _|  _| *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| \___/_.__// \___\__|\__| *
        //*                                                |__/             *
        //-MARK: import definition object
        class import_definition_object final : public import_rule {
        public:
            using names_pointer = not_null<import_name_mapping const *>;
            using definition_container_pointer = not_null<import_definition::container *>;

            import_definition_object (parse_stack_pointer const stack,
                                      definition_container_pointer const definitions,
                                      names_pointer const names, database const & db,
                                      fragment_index_pointer const & fragments);
            import_definition_object (import_definition_object const &) = delete;
            import_definition_object (import_definition_object &&) noexcept = delete;
            ~import_definition_object () noexcept override = default;

            import_definition_object & operator= (import_definition_object const &) = delete;
            import_definition_object & operator= (import_definition_object &&) noexcept = delete;

            gsl::czstring name () const noexcept override;

            std::error_code begin_object () override;
            std::error_code end_array () override;

        private:
            definition_container_pointer const definitions_;
            names_pointer const names_;
            database const & db_;
            fragment_index_pointer const fragments_;
        };

        //*                    _ _      _   _           *
        //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
        //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
        //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
        //*              |_|                            *
        //-MARK: import compilation
        template <typename TransactionLock>
        class import_compilation final : public import_rule {
        public:
            using transaction_pointer = not_null<transaction<TransactionLock> *>;
            using names_pointer = not_null<import_name_mapping const *>;

            import_compilation (parse_stack_pointer const stack,
                                transaction_pointer const transaction, names_pointer const names,
                                fragment_index_pointer const & fragments,
                                index::digest const digest)
                    : import_rule (stack)
                    , transaction_{transaction}
                    , names_{names}
                    , fragments_{fragments}
                    , digest_{digest} {}
            import_compilation (import_compilation const &) = delete;
            import_compilation (import_compilation &&) noexcept = delete;
            ~import_compilation () noexcept override = default;

            import_compilation & operator= (import_compilation const &) = delete;
            import_compilation & operator== (import_compilation &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "compilation"; }

            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            transaction_pointer const transaction_;
            names_pointer const names_;
            fragment_index_pointer const fragments_;
            index::digest const digest_;

            enum { path_index, triple_index };
            std::bitset<triple_index + 1> seen_;

            std::uint64_t path_ = 0;
            std::uint64_t triple_ = 0;
            import_definition::container definitions_;
        };

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code import_compilation<TransactionLock>::key (std::string const & k) {
            if (k == "path") {
                seen_[path_index] = true;
                return push<uint64_rule> (&path_);
            }
            if (k == "triple") {
                seen_[triple_index] = true;
                return push<uint64_rule> (&triple_);
            }
            if (k == "definitions") {
                return push_array_rule<import_definition_object> (this, &definitions_, names_,
                                                                  transaction_->db (), fragments_);
            }
            return import_error::unknown_compilation_object_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code import_compilation<TransactionLock>::end_object () {
            if (!seen_.all ()) {
                return import_error::incomplete_compilation_object;
            }
            auto const path = names_->lookup (path_);
            if (!path) {
                return path.get_error ();
            }
            auto const triple = names_->lookup (triple_);
            if (!triple) {
                return triple.get_error ();
            }

            // Create the compilation record in the store.
            extent<repo::compilation> const compilation_extent = repo::compilation::alloc (
                *transaction_, *path, *triple, std::begin (definitions_), std::end (definitions_));

            // Insert this compilation into the compilations index.
            auto compilations = pstore::index::get_index<pstore::trailer::indices::compilation> (
                transaction_->db ());
            compilations->insert (*transaction_, std::make_pair (digest_, compilation_extent));

            return pop ();
        }

        //*                    _ _      _   _               _         _          *
        //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _  ___ (_)_ _  __| |_____ __ *
        //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \(_-< | | ' \/ _` / -_) \ / *
        //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_/__/ |_|_||_\__,_\___/_\_\ *
        //*              |_|                                                     *
        //-MARK: compilation index
        template <typename TransactionLock>
        class import_compilations_index final : public import_rule {
        public:
            using transaction_pointer = transaction<TransactionLock> *;
            using names_pointer = import_name_mapping const *;

            import_compilations_index (parse_stack_pointer const stack,
                                       transaction_pointer const transaction,
                                       names_pointer const names);
            import_compilations_index (import_compilations_index const &) = delete;
            import_compilations_index (import_compilations_index &&) noexcept = delete;
            ~import_compilations_index () noexcept override = default;

            import_compilations_index & operator= (import_compilations_index const &) = delete;
            import_compilations_index & operator= (import_compilations_index &&) = delete;

            gsl::czstring name () const noexcept override;
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            transaction_pointer const transaction_;
            names_pointer const names_;
            fragment_index_pointer const fragments_;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        import_compilations_index<TransactionLock>::import_compilations_index (
            parse_stack_pointer const stack, transaction_pointer const transaction,
            names_pointer const names)
                : import_rule (stack)
                , transaction_{transaction}
                , names_{names}
                , fragments_{index::get_index<trailer::indices::fragment> (transaction->db ())} {}

        // name
        // ~~~~
        template <typename TransactionLock>
        gsl::czstring import_compilations_index<TransactionLock>::name () const noexcept {
            return "compilations index";
        }

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code import_compilations_index<TransactionLock>::key (std::string const & s) {
            if (maybe<index::digest> const digest = digest_from_string (s)) {
                return push_object_rule<import_compilation<TransactionLock>> (
                    this, transaction_, names_, fragments_, index::digest{*digest});
            }
            return import_error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code import_compilations_index<TransactionLock>::end_object () {
            return pop ();
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_COMPILATION_HPP
