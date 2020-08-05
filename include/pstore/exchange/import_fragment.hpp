//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- include/pstore/exchange/import_fragment.hpp ------------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP
#define PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP

#include <cassert>
#include <vector>

#include "pstore/core/index_types.hpp"
#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {

        //*     _     _                _ _                       _   _           *
        //*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
        //* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
        //*                    |___/                                             *
        //-MARK: debug line section
        template <typename OutputIterator>
        class import_debug_line_section final : public import_generic_section<OutputIterator> {
        public:
            using names_pointer = not_null<import_name_mapping const *>;

            import_debug_line_section (rule::parse_stack_pointer const stack,
                                       repo::section_kind kind, database & db,
                                       names_pointer const names,
                                       repo::section_content * const content,
                                       OutputIterator * const out)
                    : import_generic_section<OutputIterator> (stack, kind, db, names, content, out)
                    , db_{db}
                    , out_{out} {
                assert (kind == repo::section_kind::debug_line);
            }
            import_debug_line_section (import_debug_line_section const &) = delete;
            import_debug_line_section (import_debug_line_section &&) noexcept = delete;

            import_debug_line_section & operator= (import_debug_line_section const &) = delete;
            import_debug_line_section & operator= (import_debug_line_section &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "debug line section"; }
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            enum { header };
            std::bitset<header + 1> seen_;

            database & db_;
            std::string header_digest_;
            not_null<OutputIterator *> const out_;
        };

        // key
        // ~~~
        template <typename OutputIterator>
        std::error_code import_debug_line_section<OutputIterator>::key (std::string const & k) {
            if (k == "header") {
                seen_[header] = true;
                return this->template push<string_rule> (&header_digest_);
            }
            return import_generic_section<OutputIterator>::key (k);
        }

        // end object
        // ~~~~~~~~~~
        template <typename OutputIterator>
        std::error_code import_debug_line_section<OutputIterator>::end_object () {
            maybe<index::digest> const digest = exchange::digest_from_string (header_digest_);
            if (!digest) {
                return import_error::bad_digest;
            }
            if (!seen_.all ()) {
                return import_error::incomplete_debug_line_section;
            }

            auto const index = index::get_index<trailer::indices::debug_line_header> (db_);
            auto pos = index->find (db_, *digest);
            if (pos == index->end (db_)) {
                return import_error::debug_line_header_digest_not_found;
            }
            auto const header_extent = pos->second;

            if (error_or<repo::section_content *> const content = this->content_object ()) {
                *out_ = std::make_unique<repo::debug_line_section_creation_dispatcher> (
                    *digest, header_extent, content.get ());
                return this->pop ();
            } else {
                return content.get_error ();
            }
        }


        namespace details {

            // FIXME: This implementation is just a placeholder to allow the code to compile.
            template <typename OutputIterator>
            class import_bss_section : public rule {
            public:
                using names_pointer = not_null<import_name_mapping const *>;
                using content_pointer = not_null<repo::section_content *>;

                import_bss_section (parse_stack_pointer const stack, repo::section_kind kind,
                                    database const &, names_pointer const names,
                                    content_pointer const content,
                                    not_null<OutputIterator *> const out) noexcept
                        : rule (stack) {}

                gsl::czstring name () const noexcept override { return "BSS section"; }
            };

            // FIXME: This implementation is just a placeholder to allow the code to compile.
            template <typename OutputIterator>
            class import_dependents_section : public rule {
            public:
                using names_pointer = not_null<import_name_mapping const *>;
                using content_pointer = not_null<repo::section_content *>;

                import_dependents_section (parse_stack_pointer const stack, repo::section_kind kind,
                                           database const &, names_pointer const names,
                                           content_pointer const content,
                                           not_null<OutputIterator *> const out) noexcept
                        : rule (stack) {}

                gsl::czstring name () const noexcept override { return "dependents section"; }
            };

            /// We can map from the section_kind enum to the type of data used to represent a
            /// section of that kind. section_to_importer is used to convert from this type class
            /// that we use to import that data. For example, the text section is represented by the
            /// generic_section type. We then use the import_generic_section class to import it.
            /// There are template specializations for each of the section content types in the
            /// database.
            template <typename Section, typename OutputIterator>
            struct section_to_importer {};
            template <typename Section, typename OutputIterator>
            using section_to_importer_t =
                typename section_to_importer<Section, OutputIterator>::type;

            template <typename OutputIterator>
            struct section_to_importer<repo::generic_section, OutputIterator> {
                using type = import_generic_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::bss_section, OutputIterator> {
                using type = import_bss_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::dependents, OutputIterator> {
                using type = import_dependents_section<OutputIterator>;
            };
            template <typename OutputIterator>
            struct section_to_importer<repo::debug_line_section, OutputIterator> {
                using type = import_debug_line_section<OutputIterator>;
            };

        } // end namespace details


        //*   __                             _                _   _              *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
        //*              |___/                                                   *
        //-MARK: fragment sections
        template <typename TransactionLock>
        class fragment_sections final : public rule {
        public:
            using transaction_pointer = not_null<transaction<TransactionLock> *>;
            using names_pointer = not_null<import_name_mapping const *>;

            fragment_sections (parse_stack_pointer const stack,
                               transaction_pointer const transaction, names_pointer const names,
                               index::digest const * const digest)
                    : rule (stack)
                    , transaction_{transaction}
                    , names_{names}
                    , digest_{digest}
                    , oit_{dispatchers_} {}
            gsl::czstring name () const noexcept override { return "fragment sections"; }
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            transaction_pointer const transaction_;
            names_pointer const names_;
            index::digest const * const digest_;

            repo::section_content * section_contents (repo::section_kind kind) noexcept {
                return &contents_[static_cast<std::underlying_type<repo::section_kind>::type> (
                    kind)];
            }

            std::array<repo::section_content, repo::num_section_kinds> contents_;
            std::vector<std::unique_ptr<repo::section_creation_dispatcher>> dispatchers_;
            std::back_insert_iterator<decltype (dispatchers_)> oit_;
        };

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code fragment_sections<TransactionLock>::key (std::string const & s) {
            using repo::section_kind;

#define X(a) {#a, section_kind::a},
            static std::unordered_map<std::string, section_kind> const map{
                PSTORE_MCREPO_SECTION_KINDS};
#undef X
            auto const pos = map.find (s);
            if (pos == map.end ()) {
                return import_error::unknown_section_name;
            }

#define X(a)                                                                                       \
case section_kind::a:                                                                              \
    return push_object_rule<details::section_to_importer_t<                                        \
        repo::enum_to_section_t<section_kind::a>, decltype (oit_)>> (                              \
        this, pos->second, transaction_->db (), names_, section_contents (pos->second), &oit_);

            switch (pos->second) {
                PSTORE_MCREPO_SECTION_KINDS
            case section_kind::last: assert (false && "Illegal section kind"); // unreachable
            }
#undef X
            return import_error::unknown_section_name;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code fragment_sections<TransactionLock>::end_object () {
            std::shared_ptr<index::fragment_index> const index =
                index::get_index<trailer::indices::fragment> (transaction_->db (),
                                                              true /* create */);
            index->insert (
                *transaction_,
                std::make_pair (
                    *digest_, repo::fragment::alloc (*transaction_,
                                                     make_pointee_adaptor (dispatchers_.begin ()),
                                                     make_pointee_adaptor (dispatchers_.end ()))));
            return pop ();
        }


        //*   __                             _     _         _          *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  (_)_ _  __| |_____ __ *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| | | ' \/ _` / -_) \ / *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| |_|_||_\__,_\___/_\_\ *
        //*              |___/                                          *
        //-MARK: fragment index
        template <typename TransactionLock>
        class fragment_index final : public rule {
        public:
            using transaction_pointer = not_null<transaction<TransactionLock> *>;
            using names_pointer = not_null<import_name_mapping const *>;

            fragment_index (parse_stack_pointer const stack, transaction_pointer const transaction,
                            names_pointer const names);
            fragment_index (fragment_index const &) = delete;
            fragment_index (fragment_index &&) noexcept = delete;

            fragment_index & operator= (fragment_index const &) = delete;
            fragment_index & operator= (fragment_index &&) noexcept = delete;

            gsl::czstring name () const noexcept override;
            std::error_code key (std::string const & s) override;
            std::error_code end_object () override;

        private:
            transaction_pointer const transaction_;
            names_pointer const names_;

            std::vector<std::unique_ptr<repo::section_creation_dispatcher>> sections_;
            index::digest digest_;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        fragment_index<TransactionLock>::fragment_index (parse_stack_pointer const stack,
                                                         transaction_pointer const transaction,
                                                         names_pointer const names)
                : rule (stack)
                , transaction_{transaction}
                , names_{names} {
            sections_.reserve (
                static_cast<std::underlying_type_t<repo::section_kind>> (repo::section_kind::last));
        }

        // name
        // ~~~~
        template <typename TransactionLock>
        gsl::czstring fragment_index<TransactionLock>::name () const noexcept {
            return "fragment index";
        }

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code fragment_index<TransactionLock>::key (std::string const & s) {
            if (maybe<index::digest> const digest = digest_from_string (s)) {
                digest_ = *digest;
                return push_object_rule<fragment_sections<TransactionLock>> (this, transaction_,
                                                                             names_, &digest_);
            }
            return import_error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code fragment_index<TransactionLock>::end_object () {
            return pop ();
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP
