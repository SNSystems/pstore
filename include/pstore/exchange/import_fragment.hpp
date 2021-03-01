//===- include/pstore/exchange/import_fragment.hpp --------*- mode: C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_fragment.hpp
/// \brief Rules for importing the fragment index.

#ifndef PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP
#define PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP

#include "pstore/exchange/import_section_to_importer.hpp"
#include "pstore/mcrepo/fragment.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            //*          _    _                            _      _     *
            //*  __ _ __| |__| |_ _ ___ ______  _ __  __ _| |_ __| |_   *
            //* / _` / _` / _` | '_/ -_|_-<_-< | '_ \/ _` |  _/ _| ' \  *
            //* \__,_\__,_\__,_|_| \___/__/__/ | .__/\__,_|\__\__|_||_| *
            //*                                |_|                      *
            class address_patch final : public patcher {
            public:
                address_patch (gsl::not_null<database *> const db,
                               extent<repo::fragment> const & ex) noexcept;
                address_patch (address_patch const &) = delete;
                address_patch (address_patch &&) = delete;

                ~address_patch () noexcept override = default;

                address_patch & operator= (address_patch const &) = delete;
                address_patch & operator= (address_patch &&) = delete;

                std::error_code operator() (transaction_base * const transaction) override;

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
            class fragment_sections final : public rule {
            public:
                fragment_sections (not_null<context *> const ctxt,
                                   not_null<transaction_base *> const transaction,
                                   not_null<name_mapping const *> const names,
                                   not_null<index::digest const *> const digest);

                gsl::czstring name () const noexcept override;
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                not_null<transaction_base *> const transaction_;
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

                repo::section_content * section_contents (repo::section_kind const kind) noexcept {
                    return &contents_[static_cast<std::underlying_type<repo::section_kind>::type> (
                        kind)];
                }
            };

            //*   __                             _     _         _          *
            //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  (_)_ _  __| |_____ __ *
            //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| | | ' \/ _` / -_) \ / *
            //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| |_|_||_\__,_\___/_\_\ *
            //*              |___/                                          *
            //-MARK: fragment index
            class fragment_index final : public rule {
            public:
                fragment_index (not_null<context *> ctxt, not_null<transaction_base *> transaction,
                                not_null<name_mapping const *> names);
                fragment_index (fragment_index const &) = delete;
                fragment_index (fragment_index &&) noexcept = delete;

                ~fragment_index () noexcept override = default;

                fragment_index & operator= (fragment_index const &) = delete;
                fragment_index & operator= (fragment_index &&) noexcept = delete;

                gsl::czstring name () const noexcept override;
                std::error_code key (std::string const & s) override;
                std::error_code end_object () override;

            private:
                not_null<transaction_base *> const transaction_;
                not_null<name_mapping const *> const names_;

                std::vector<std::unique_ptr<repo::section_creation_dispatcher>> sections_;
                index::digest digest_;
            };

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FRAGMENT_HPP
