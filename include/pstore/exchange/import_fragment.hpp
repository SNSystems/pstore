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
#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            class address_patch final : public patcher {
            public:
                address_patch (gsl::not_null<database *> const db,
                               extent<repo::fragment> const & fragment_extent)
                        : db_{db}
                        , fragment_extent_{fragment_extent} {}
                address_patch (address_patch const &) = delete;
                address_patch (address_patch &&) = delete;

                ~address_patch () noexcept override = default;

                address_patch & operator= (address_patch const &) = delete;
                address_patch & operator= (address_patch &&) = delete;

                std::error_code operator() (transaction_base * const transaction) override {
                    auto fragment = repo::fragment::load (*transaction, fragment_extent_);
                    assert (fragment->has_section (repo::section_kind::linked_definitions));
                    repo::linked_definitions & linked =
                        fragment->template at<repo::section_kind::linked_definitions> ();

                    auto const compilation_index =
                        pstore::index::get_index<pstore::trailer::indices::compilation> (*db_);

                    for (repo::linked_definitions::value_type & l : linked) {
                        auto const pos = compilation_index->find (*db_, l.compilation);
                        if (pos == compilation_index->end (*db_)) {
                            // compilation was not found.
                            return {error::no_such_compilation};
                        }
                        auto const compilation =
                            repo::compilation::load (transaction->db (), pos->second);
                        if (l.index >= compilation->size ()) {
                            return {error::index_out_of_range};
                        }
                        // Compute the offset of the link.index definition from the start of the
                        // compilation's storage (c).
                        auto const offset =
                            reinterpret_cast<std::uintptr_t> (&(*compilation)[l.index]) -
                            reinterpret_cast<std::uintptr_t> (compilation.get ());
                        // Compute the address of the link.index definition.
                        l.pointer = typed_address<repo::compilation_member>::make (
                            pos->second.addr.to_address () + offset);
                    }
                    return {};
                }

            private:
                gsl::not_null<database *> const db_;
                extent<repo::fragment> const fragment_extent_;
            };



            //*   __                             _                _   _              *
            //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
            //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
            //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
            //*              |___/                                                   *
            //-MARK: fragment sections
            template <typename TransactionLock>
            class fragment_sections final : public rule {
            public:
                fragment_sections (not_null<context *> const ctxt,
                                   not_null<transaction<TransactionLock> *> const transaction,
                                   not_null<name_mapping const *> const names,
                                   not_null<index::digest const *> const digest)
                        : rule (ctxt)
                        , transaction_{transaction}
                        , names_{names}
                        , digest_{digest}
                        , oit_{dispatchers_} {
                    assert (&transaction->db () == ctxt->db);
                }

                gsl::czstring name () const noexcept override { return "fragment sections"; }
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                not_null<transaction<TransactionLock> *> const transaction_;
                not_null<name_mapping const *> const names_;
                not_null<index::digest const *> const digest_;

                std::array<repo::section_content, repo::num_section_kinds> contents_;
                linked_definitions_container linked_definitions_;

                std::vector<std::unique_ptr<repo::section_creation_dispatcher>> dispatchers_;
                std::back_insert_iterator<decltype (dispatchers_)> oit_;

                // (For explicit specialization, you need to specialize the outer class before the
                // inner but I don't want to do that here. A workaround is to rely on partial
                // specialization by adding the 'Dummy' template parameter.)
                template <repo::section_kind Kind, typename Dummy = void>
                struct section_importer_creator {
                    std::error_code operator() (fragment_sections * const fs) const {
                        using importer =
                            section_to_importer_t<repo::enum_to_section_t<Kind>, decltype (oit_)>;
                        return push_object_rule<importer> (fs, Kind, fs->names_,
                                                           fs->section_contents (Kind), &fs->oit_);
                    }
                };

                template <typename Dummy>
                struct section_importer_creator<repo::section_kind::linked_definitions, Dummy> {
                    std::error_code operator() (fragment_sections * const fs) const {
                        using importer = linked_definitions_section<decltype (oit_)>;
                        return push_array_rule<importer> (fs, &fs->linked_definitions_, &fs->oit_);
                    }
                };

                template <repo::section_kind Kind>
                std::error_code create_section_importer () {
                    return section_importer_creator<Kind>{}(this);
                }

                constexpr repo::section_content *
                section_contents (repo::section_kind const kind) noexcept {
                    return &contents_[static_cast<std::underlying_type<repo::section_kind>::type> (
                        kind)];
                }
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
                return error::unknown_section_name;
            }

#define X(a)                                                                                       \
case section_kind::a: return this->create_section_importer<section_kind::a> ();
            switch (pos->second) {
                PSTORE_MCREPO_SECTION_KINDS
            case section_kind::last: assert (false && "Illegal section kind"); // unreachable
            }
#undef X
            return error::unknown_section_name;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code fragment_sections<TransactionLock>::end_object () {
            context * const ctxt = this->get_context ();
            assert (ctxt->db == &transaction_->db ());

            auto const dispatchers_begin = make_pointee_adaptor (dispatchers_.begin ());
            auto const dispatchers_end = make_pointee_adaptor (dispatchers_.end ());

            auto const fext =
                repo::fragment::alloc (*transaction_, dispatchers_begin, dispatchers_end);
            auto const fragment_index =
                index::get_index<trailer::indices::fragment> (*ctxt->db, true /* create */);
            fragment_index->insert (*transaction_, std::make_pair (*digest_, fext));

            // If this fragment has a linked-definitions section then we need to patch the addresses
            // of the referenced definitions one we've imported everything.
            if (std::find_if (dispatchers_begin, dispatchers_end,
                              [] (repo::section_creation_dispatcher const & d) {
                                  return d.kind () == repo::section_kind::linked_definitions;
                              }) != dispatchers_end) {
                ctxt->patches.emplace_back (new address_patch (ctxt->db, fext));
            }
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
            using names_pointer = not_null<name_mapping const *>;

            fragment_index (not_null<context *> ctxt, transaction_pointer transaction,
                            names_pointer names);
            fragment_index (fragment_index const &) = delete;
            fragment_index (fragment_index &&) noexcept = delete;

            ~fragment_index () noexcept override = default;

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
        fragment_index<TransactionLock>::fragment_index (not_null<context *> const ctxt,
                                                         transaction_pointer const transaction,
                                                         names_pointer const names)
                : rule (ctxt)
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
            if (maybe<index::digest> const digest = uint128::from_hex_string (s)) {
                digest_ = *digest;
                return push_object_rule<fragment_sections<TransactionLock>> (this, transaction_,
                                                                             names_, &digest_);
            }
            return error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code fragment_index<TransactionLock>::end_object () {
            return pop ();
        }

        } // end namespace import
    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP
