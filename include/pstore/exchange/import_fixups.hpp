//*  _                            _      __ _                       *
//* (_)_ __ ___  _ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*             |_|                                     |_|         *
//===- include/pstore/exchange/import_fixups.hpp --------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file import_fixups.hpp
/// \brief  Declares the rules for importing internal- and external-fixup records.
#ifndef PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
#define PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP

#include <bitset>

#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            namespace details {

                //*             _   _                                 *
                //*  ___ ___ __| |_(_)___ _ _    _ _  __ _ _ __  ___  *
                //* (_-</ -_) _|  _| / _ \ ' \  | ' \/ _` | '  \/ -_) *
                //* /__/\___\__|\__|_\___/_||_| |_||_\__,_|_|_|_\___| *
                //*                                                   *
                //-MARK: section name
                class section_name final : public rule {
                public:
                    section_name (not_null<context *> const ctxt,
                                  not_null<pstore::repo::section_kind *> const section)
                            : rule (ctxt)
                            , section_{section} {}
                    section_name (section_name const &) = delete;
                    section_name (section_name &&) noexcept = delete;

                    ~section_name () noexcept override = default;

                    section_name & operator= (section_name const &) = delete;
                    section_name & operator= (section_name &&) noexcept = delete;

                    gsl::czstring name () const noexcept override { return "section name"; }
                    std::error_code string_value (std::string const & s) override;

                private:
                    not_null<repo::section_kind *> const section_;
                };

            } // end namespace details


            //*  _     _                     _    __ _                *
            //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
            //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
            //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
            //*                                                |_|    *
            //-MARK:internal fixup
            class internal_fixup final : public rule {
            public:
                using fixups_pointer = not_null<std::vector<repo::internal_fixup> *>;

                internal_fixup (not_null<context *> ctxt, not_null<name_mapping const *> names,
                                fixups_pointer fixups);
                internal_fixup (internal_fixup const &) = delete;
                internal_fixup (internal_fixup &&) noexcept = delete;

                ~internal_fixup () noexcept override = default;

                internal_fixup & operator= (internal_fixup const &) = delete;
                internal_fixup & operator= (internal_fixup &&) noexcept = delete;

                gsl::czstring name () const noexcept override;
                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            private:
                enum { section, type, offset, addend };
                std::bitset<addend + 1> seen_;

                fixups_pointer const fixups_;

                repo::section_kind section_ = repo::section_kind::last;
                std::uint64_t type_ = 0;
                std::uint64_t offset_ = 0;
                std::int64_t addend_ = 0;
            };


            //*          _                     _    __ _                *
            //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
            //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
            //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
            //*                                                  |_|    *
            //-MARK:external fixup
            class external_fixup final : public rule {
            public:
                using fixups_pointer = not_null<std::vector<repo::external_fixup> *>;

                external_fixup (not_null<context *> ctxt, not_null<name_mapping const *> names,
                                fixups_pointer fixups);
                external_fixup (external_fixup const &) = delete;
                external_fixup (external_fixup &&) noexcept = delete;

                ~external_fixup () noexcept override = default;

                external_fixup & operator= (external_fixup const &) = delete;
                external_fixup & operator= (external_fixup &&) noexcept = delete;

                gsl::czstring name () const noexcept override;
                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            private:
                enum { name_index, type, is_weak, offset, addend };
                std::bitset<addend + 1> seen_;
                not_null<name_mapping const *> names_;
                fixups_pointer const fixups_;

                std::uint64_t name_ = 0;
                std::uint64_t type_ = 0;
                bool is_weak_ = false;
                std::uint64_t offset_ = 0;
                std::int64_t addend_ = 0;
            };

            //*   __ _                        _     _        _    *
            //*  / _(_)_ ___  _ _ __ ___  ___| |__ (_)___ __| |_  *
            //* |  _| \ \ / || | '_ (_-< / _ \ '_ \| / -_) _|  _| *
            //* |_| |_/_\_\\_,_| .__/__/ \___/_.__// \___\__|\__| *
            //*                |_|               |__/             *
            //-MARK: fixups object
            template <typename Next, typename Fixup>
            class fixups_object final : public rule {
            public:
                using fixups_pointer = not_null<std::vector<Fixup> *>;

                fixups_object (not_null<context *> const ctxt,
                               not_null<name_mapping const *> const names,
                               fixups_pointer const fixups)
                        : rule (ctxt)
                        , names_{names}
                        , fixups_{fixups} {}
                fixups_object (fixups_object const &) = delete;
                fixups_object (fixups_object &&) noexcept = delete;

                ~fixups_object () noexcept override = default;

                fixups_object & operator= (fixups_object const &) = delete;
                fixups_object & operator= (fixups_object &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "fixups object"; }
                std::error_code begin_object () override {
                    return this->push<Next> (names_, fixups_);
                }
                std::error_code end_array () override { return pop (); }

            private:
                not_null<name_mapping const *> const names_;
                fixups_pointer const fixups_;
            };

            using ifixups_object = fixups_object<internal_fixup, repo::internal_fixup>;
            using xfixups_object = fixups_object<external_fixup, repo::external_fixup>;

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
