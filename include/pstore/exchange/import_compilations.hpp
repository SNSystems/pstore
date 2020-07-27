//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*                            _ _       _   _                  *
//*   ___ ___  _ __ ___  _ __ (_) | __ _| |_(_) ___  _ __  ___  *
//*  / __/ _ \| '_ ` _ \| '_ \| | |/ _` | __| |/ _ \| '_ \/ __| *
//* | (_| (_) | | | | | | |_) | | | (_| | |_| | (_) | | | \__ \ *
//*  \___\___/|_| |_| |_| .__/|_|_|\__,_|\__|_|\___/|_| |_|___/ *
//*                     |_|                                     *
//===- include/pstore/exchange/import_compilations.hpp --------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP
#define PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP

#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_names.hpp"

#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/compilation.hpp"

namespace pstore {
    namespace exchange {

        using fragment_index_pointer = gsl::not_null<std::shared_ptr<index::fragment_index>>;

        //*     _      __ _      _ _   _           *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _   *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| *
        //*                                        *
        //-MARK: definition
        template <typename TransactionLock>
        class definition final : public rule {
        public:
            using transaction_type = transaction<TransactionLock>;
            using transaction_pointer = transaction_type *;
            using names_pointer = names<TransactionLock> *;

            using container = std::vector<repo::compilation_member>;

            definition (parse_stack_pointer const stack,
                        gsl::not_null<container *> const definitions, names_pointer const names,
                        transaction_pointer const transaction,
                        fragment_index_pointer const & fragments)
                    : rule (stack)
                    , definitions_{definitions}
                    , names_{names}
                    , transaction_{transaction}
                    , fragments_{fragments} {}
            definition (definition const &) = delete;
            definition (definition &&) noexcept = delete;

            definition & operator= (definition const &) = delete;
            definition & operator= (definition &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "definition"; }

            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

            static maybe<repo::linkage> decode_linkage (std::string const & linkage);
            static maybe<repo::visibility> decode_visibility (std::string const & visibility);

        private:
            gsl::not_null<container *> const definitions_;
            names_pointer const names_;
            transaction_pointer const transaction_;
            fragment_index_pointer const fragments_;

            std::string digest_;
            std::uint64_t name_ = 0;
            std::string linkage_;
            std::string visibility_;
        };

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code definition<TransactionLock>::key (std::string const & k) {
            if (k == "digest") {
                return push<string_rule> (&digest_);
            }
            if (k == "name") {
                return push<uint64_rule> (&name_);
            }
            if (k == "linkage") {
                return push<string_rule> (&linkage_);
            }
            if (k == "visibility") {
                return push<string_rule> (&visibility_);
            }
            return import_error::unknown_definition_object_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code definition<TransactionLock>::end_object () {
            auto const digest = digest_from_string (digest_);
            if (!digest) {
                return import_error::bad_digest;
            }
            auto const fpos = fragments_->find (transaction_->db (), *digest);
            if (fpos == fragments_->end (transaction_->db ())) {
                return import_error::no_such_fragment;
            }

            auto const linkage = decode_linkage (linkage_);
            if (!linkage) {
                return import_error::bad_linkage;
            }
            auto const visibility = decode_visibility (visibility_);
            if (!visibility) {
                return import_error::bad_visibility;
            }

            auto const name = names_->lookup (name_);
            if (!name) {
                return name.get_error ();
            }

            definitions_->emplace_back (*digest, fpos->second, *name, *linkage, *visibility);

            return pop ();
        }

        // decode linkage
        // ~~~~~~~~~~~~~~
        template <typename TransactionLock>
        maybe<repo::linkage>
        definition<TransactionLock>::decode_linkage (std::string const & linkage) {
#define X(a)                                                                                       \
    if (linkage == #a) {                                                                           \
        return just (repo::linkage::a);                                                            \
    }
            PSTORE_REPO_LINKAGES
#undef X
            return nothing<repo::linkage> ();
        }

        // decode visibility
        // ~~~~~~~~~~~~~~~~~
        template <typename TransactionLock>
        maybe<repo::visibility>
        definition<TransactionLock>::decode_visibility (std::string const & visibility) {
            if (visibility == "default") {
                return just (repo::visibility::default_vis);
            }
            if (visibility == "hidden") {
                return just (repo::visibility::hidden_vis);
            }
            if (visibility == "protected") {
                return just (repo::visibility::protected_vis);
            }
            return nothing<repo::visibility> ();
        }

        //*     _      __ _      _ _   _                _     _        _    *
        //*  __| |___ / _(_)_ _ (_) |_(_)___ _ _    ___| |__ (_)___ __| |_  *
        //* / _` / -_)  _| | ' \| |  _| / _ \ ' \  / _ \ '_ \| / -_) _|  _| *
        //* \__,_\___|_| |_|_||_|_|\__|_\___/_||_| \___/_.__// \___\__|\__| *
        //*                                                |__/             *
        //-MARK: definition object
        template <typename TransactionLock>
        class definition_object final : public rule {
        public:
            using transaction_type = transaction<TransactionLock>;
            using transaction_pointer = transaction_type *;
            using names_pointer = names<TransactionLock> *;
            using definition_container_pointer =
                gsl::not_null<typename definition<TransactionLock>::container *>;

            definition_object (parse_stack_pointer const stack,
                               definition_container_pointer const definitions,
                               names_pointer const names, transaction_pointer const transaction,
                               fragment_index_pointer const & fragments)
                    : rule (stack)
                    , definitions_{definitions}
                    , names_{names}
                    , transaction_{transaction}
                    , fragments_{fragments} {}
            definition_object (definition_object const &) = delete;
            definition_object (definition_object &&) noexcept = delete;

            definition_object & operator= (definition_object const &) = delete;
            definition_object & operator= (definition_object &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "definition_object"; }

            std::error_code begin_object () override {
                return this->push<definition<TransactionLock>> (definitions_, names_, transaction_,
                                                                fragments_);
            }
            std::error_code end_array () override { return pop (); }

        private:
            definition_container_pointer const definitions_;
            names_pointer const names_;
            transaction_pointer const transaction_;
            fragment_index_pointer const fragments_;
        };

        //*                    _ _      _   _           *
        //*  __ ___ _ __  _ __(_) |__ _| |_(_)___ _ _   *
        //* / _/ _ \ '  \| '_ \ | / _` |  _| / _ \ ' \  *
        //* \__\___/_|_|_| .__/_|_\__,_|\__|_\___/_||_| *
        //*              |_|                            *
        //-MARK: compilation
        template <typename TransactionLock>
        class compilation final : public rule {
        public:
            using transaction_type = transaction<TransactionLock>;
            using transaction_pointer = transaction_type *;
            using names_pointer = names<TransactionLock> *;

            compilation (parse_stack_pointer const stack, transaction_pointer const transaction,
                         names_pointer const names, fragment_index_pointer const & fragments,
                         index::digest const digest)
                    : rule (stack)
                    , transaction_{transaction}
                    , names_{names}
                    , fragments_{fragments}
                    , digest_{digest} {}
            compilation (compilation const &) = delete;
            compilation (compilation &&) noexcept = delete;

            compilation & operator= (compilation const &) = delete;
            compilation & operator== (compilation &&) noexcept = delete;

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
            typename definition<TransactionLock>::container definitions_;
        };

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code compilation<TransactionLock>::key (std::string const & k) {
            if (k == "path") {
                seen_[path_index] = true;
                return push<uint64_rule> (&path_);
            }
            if (k == "triple") {
                seen_[triple_index] = true;
                return push<uint64_rule> (&triple_);
            }
            if (k == "definitions") {
                return push_array_rule<definition_object<TransactionLock>> (
                    this, &definitions_, names_, transaction_, fragments_);
            }
            return import_error::unknown_compilation_object_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code compilation<TransactionLock>::end_object () {
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
        class compilations_index final : public rule {
        public:
            using transaction_type = transaction<TransactionLock>;
            using transaction_pointer = transaction_type *;
            using names_pointer = names<TransactionLock> *;

            compilations_index (parse_stack_pointer s, transaction_pointer transaction,
                                names_pointer names);
            compilations_index (compilations_index const & ) = delete;
            compilations_index (compilations_index && ) noexcept = delete;

            compilations_index & operator= (compilations_index const & ) = delete;
            compilations_index & operator= (compilations_index && ) = delete;

            gsl::czstring name () const noexcept override;
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            transaction_pointer const transaction_;
            names_pointer const names_;
            std::shared_ptr<index::fragment_index> const fragments_;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        compilations_index<TransactionLock>::compilations_index (
            parse_stack_pointer const stack, transaction_pointer const transaction,
            names_pointer const names)
                : rule (stack)
                , transaction_{transaction}
                , names_{names}
                , fragments_{index::get_index<trailer::indices::fragment> (transaction->db ())} {}

        // name
        // ~~~~
        template <typename TransactionLock>
        gsl::czstring compilations_index<TransactionLock>::name () const noexcept {
            return "compilations index";
        }

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code compilations_index<TransactionLock>::key (std::string const & s) {
            if (maybe<index::digest> const digest = digest_from_string (s)) {
                return push_object_rule<compilation<TransactionLock>> (
                    this, transaction_, names_, fragments_, index::digest{*digest});
            }
            return import_error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code compilations_index<TransactionLock>::end_object () {
            return pop ();
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_COMPILATIONS_HPP
