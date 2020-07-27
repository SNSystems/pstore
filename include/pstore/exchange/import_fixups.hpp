//*  _                            _      __ _                       *
//* (_)_ __ ___  _ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*             |_|                                     |_|         *
//===- lib/exchange/import_fixups.hpp -------------------------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
#define PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP

#include <bitset>

#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {

        namespace details {

            //*             _   _                                 *
            //*  ___ ___ __| |_(_)___ _ _    _ _  __ _ _ __  ___  *
            //* (_-</ -_) _|  _| / _ \ ' \  | ' \/ _` | '  \/ -_) *
            //* /__/\___\__|\__|_\___/_||_| |_||_\__,_|_|_|_\___| *
            //*                                                   *
            //-MARK: section name
            class section_name final : public pstore::exchange::rule {
            public:
                section_name (parse_stack_pointer const stack,
                              gsl::not_null<pstore::repo::section_kind *> section)
                        : rule (stack)
                        , section_{section} {}

                gsl::czstring name () const noexcept override { return "section name"; }
                std::error_code string_value (std::string const & s) override;

            private:
                gsl::not_null<repo::section_kind *> section_;
            };

        } // end namespace details


        //*  _  __ _                          _      *
        //* (_)/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
        //* | |  _| \ \ / || | '_ \ | '_| || | / -_) *
        //* |_|_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
        //*                  |_|                     *
        template <typename TransactionLock>
        class ifixup_rule final : public rule {
        public:
            using names_pointer = names<TransactionLock> *;

            ifixup_rule (parse_stack_pointer const stack, names_pointer const names,
                         gsl::not_null<std::vector<repo::internal_fixup> *> const fixups);

            gsl::czstring name () const noexcept override { return "ifixup rule"; }
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            enum { section, type, offset, addend };
            std::bitset<addend + 1> seen_;

            gsl::not_null<std::vector<repo::internal_fixup> *> const fixups_;

            repo::section_kind section_ = repo::section_kind::last;
            std::uint64_t type_ = 0;
            std::uint64_t offset_ = 0;
            std::uint64_t addend_ = 0;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        ifixup_rule<TransactionLock>::ifixup_rule (
            parse_stack_pointer const stack, names_pointer const /*names*/,
            gsl::not_null<std::vector<repo::internal_fixup> *> const fixups)
                : rule (stack)
                , fixups_{fixups} {}

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code ifixup_rule<TransactionLock>::key (std::string const & k) {
            if (k == "section") {
                seen_[section] = true;
                return this->push<details::section_name> (&section_);
            }
            if (k == "type") {
                seen_[type] = true;
                return this->push<uint64_rule> (&type_);
            }
            if (k == "offset") {
                seen_[offset] = true;
                return this->push<uint64_rule> (&offset_);
            }
            if (k == "addend") {
                seen_[addend] = true;
                return this->push<uint64_rule> (&addend_);
            }
            return import_error::unrecognized_ifixup_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code ifixup_rule<TransactionLock>::end_object () {
            if (!seen_.all ()) {
                return import_error::ifixup_object_was_incomplete;
            }
            // TODO: validate more values here.
            fixups_->emplace_back (section_, static_cast<pstore::repo::relocation_type> (type_),
                                   offset_, addend_);
            return pop ();
        }

        //*       __ _                          _      *
        //* __ __/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
        //* \ \ /  _| \ \ / || | '_ \ | '_| || | / -_) *
        //* /_\_\_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
        //*                    |_|                     *
        //-MARK:xfixup rule
        template <typename TransactionLock>
        class xfixup_rule final : public rule {
        public:
            using names_pointer = names<TransactionLock> *;

            xfixup_rule (parse_stack_pointer const stack, names_pointer const names,
                         gsl::not_null<std::vector<repo::external_fixup> *> const fixups);

            gsl::czstring name () const noexcept override { return "xfixup rule"; }
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            enum { name_index, type, offset, addend };
            names_pointer const names_;
            std::bitset<addend + 1> seen_;
            gsl::not_null<std::vector<repo::external_fixup> *> const fixups_;

            std::uint64_t name_ = 0;
            std::uint64_t type_ = 0;
            std::uint64_t offset_ = 0;
            std::uint64_t addend_ = 0;
        };

        // (ctor)
        // ~~~~~~
        template <typename TransactionLock>
        xfixup_rule<TransactionLock>::xfixup_rule (
            parse_stack_pointer const stack, names_pointer const names,
            gsl::not_null<std::vector<repo::external_fixup> *> const fixups)
                : rule (stack)
                , names_{names}
                , fixups_{fixups} {}

        // key
        // ~~~
        template <typename TransactionLock>
        std::error_code xfixup_rule<TransactionLock>::key (std::string const & k) {
            if (k == "name") {
                seen_[name_index] = true;
                return this->push<uint64_rule> (&name_);
            }
            if (k == "type") {
                seen_[type] = true;
                return this->push<uint64_rule> (&type_);
            }
            if (k == "offset") {
                seen_[offset] = true;
                return this->push<uint64_rule> (&offset_);
            }
            if (k == "addend") {
                seen_[addend] = true;
                return this->push<uint64_rule> (&addend_);
            }
            return import_error::unrecognized_xfixup_key;
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock>
        std::error_code xfixup_rule<TransactionLock>::end_object () {
            if (!seen_.all ()) {
                return import_error::xfixup_object_was_incomplete;
            }

            auto name = names_->lookup (name_);
            if (!name) {
                return name.get_error ();
            }

            // TODO: validate some values here.
            fixups_->emplace_back (*name, static_cast<repo::relocation_type> (type_), offset_,
                                   addend_);
            return pop ();
        }

        //*   __ _                        _     _        _    *
        //*  / _(_)_ ___  _ _ __ ___  ___| |__ (_)___ __| |_  *
        //* |  _| \ \ / || | '_ (_-< / _ \ '_ \| / -_) _|  _| *
        //* |_| |_/_\_\\_,_| .__/__/ \___/_.__// \___\__|\__| *
        //*                |_|               |__/             *
        //-MARK: fixups object
        template <typename TransactionLock, typename Next, typename Fixup>
        class fixups_object final : public rule {
        public:
            using names_pointer = gsl::not_null<names<TransactionLock> *>;

            fixups_object (parse_stack_pointer const stack, names_pointer const names,
                           gsl::not_null<std::vector<Fixup> *> const fixups)
                    : rule (stack)
                    , names_{names}
                    , fixups_{fixups} {}
            gsl::czstring name () const noexcept override { return "fixups object"; }
            std::error_code begin_object () override { return this->push<Next> (names_, fixups_); }
            std::error_code end_array () override { return pop (); }

        private:
            names_pointer const names_;
            gsl::not_null<std::vector<Fixup> *> const fixups_;
        };

        template <typename TransactionLock>
        using ifixups_object =
            fixups_object<TransactionLock, ifixup_rule<TransactionLock>, repo::internal_fixup>;
        template <typename TransactionLock>
        using xfixups_object =
            fixups_object<TransactionLock, xfixup_rule<TransactionLock>, repo::external_fixup>;

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
