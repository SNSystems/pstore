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
//===- lib/exchange/import_fragment.cpp -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "pstore/exchange/import_fragment.hpp"

#include <type_traits>

namespace pstore {
    namespace exchange {
        namespace import {

            //*          _    _                            _      _     *
            //*  __ _ __| |__| |_ _ ___ ______  _ __  __ _| |_ __| |_   *
            //* / _` / _` / _` | '_/ -_|_-<_-< | '_ \/ _` |  _/ _| ' \  *
            //* \__,_\__,_\__,_|_| \___/__/__/ | .__/\__,_|\__\__|_||_| *
            //*                                |_|                      *
            //-MARK: address patch
            // ctor
            // ~~~~
            address_patch::address_patch (gsl::not_null<database *> const db,
                                          extent<repo::fragment> const & ex) noexcept
                    : db_{db}
                    , fragment_extent_{ex} {}

            // operator()
            // ~~~~~~~~~~
            std::error_code address_patch::operator() (transaction_base * const transaction) {
                auto const compilations = index::get_index<trailer::indices::compilation> (*db_);

                auto const fragment = repo::fragment::load (*transaction, fragment_extent_);
                for (repo::linked_definitions::value_type & l :
                     fragment->template at<repo::section_kind::linked_definitions> ()) {
                    auto const pos = compilations->find (*db_, l.compilation);
                    if (pos == compilations->end (*db_)) {
                        return error::no_such_compilation;
                    }
                    auto const compilation =
                        repo::compilation::load (transaction->db (), pos->second);
                    static_assert (std::is_unsigned<decltype (l.index)>::value,
                                   "index is not unsigned so we'll have to check for negative");
                    if (l.index >= compilation->size ()) {
                        return error::index_out_of_range;
                    }

                    l.pointer = repo::compilation::index_address (pos->second.addr, l.index);
                }
                return {};
            }

            //*   __                             _                _   _              *
            //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
            //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
            //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
            //*              |___/                                                   *
            //-MARK: fragment sections
            // (ctor)
            // ~~~~~~
            fragment_sections::fragment_sections (not_null<context *> const ctxt,
                                                  not_null<transaction_base *> const transaction,
                                                  not_null<name_mapping const *> const names,
                                                  not_null<index::digest const *> const digest)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names}
                    , digest_{digest}
                    , oit_{dispatchers_} {
                PSTORE_ASSERT (&transaction->db () == ctxt->db);
            }

            // name
            // ~~~~
            gsl::czstring fragment_sections::name () const noexcept { return "fragment sections"; }

            // key
            // ~~~
            std::error_code fragment_sections::key (std::string const & s) {
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
                case section_kind::last:
                    PSTORE_ASSERT (false && "Illegal section kind"); // unreachable
                }
#undef X
                return error::unknown_section_name;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code fragment_sections::end_object () {
                context * const ctxt = this->get_context ();
                PSTORE_ASSERT (ctxt->db == &transaction_->db ());

                auto const dispatchers_begin = make_pointee_adaptor (dispatchers_.begin ());
                auto const dispatchers_end = make_pointee_adaptor (dispatchers_.end ());

                auto const fext =
                    repo::fragment::alloc (*transaction_, dispatchers_begin, dispatchers_end);
                auto const fragment_index =
                    index::get_index<trailer::indices::fragment> (*ctxt->db, true /* create */);
                fragment_index->insert (*transaction_, std::make_pair (*digest_, fext));

                // If this fragment has a linked-definitions section then we need to patch the
                // addresses of the referenced definitions one we've imported everything.
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
            // (ctor)
            // ~~~~~~
            fragment_index::fragment_index (not_null<context *> const ctxt,
                                            not_null<transaction_base *> const transaction,
                                            not_null<name_mapping const *> const names)
                    : rule (ctxt)
                    , transaction_{transaction}
                    , names_{names} {
                sections_.reserve (static_cast<std::underlying_type_t<repo::section_kind>> (
                    repo::section_kind::last));
            }

            // name
            // ~~~~
            gsl::czstring fragment_index::name () const noexcept { return "fragment index"; }

            // key
            // ~~~
            std::error_code fragment_index::key (std::string const & s) {
                if (maybe<index::digest> const digest = uint128::from_hex_string (s)) {
                    digest_ = *digest;
                    return push_object_rule<fragment_sections> (this, transaction_, names_,
                                                                &digest_);
                }
                return error::bad_digest;
            }

            // end object
            // ~~~~~~~~~~
            std::error_code fragment_index::end_object () { return pop (); }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore
