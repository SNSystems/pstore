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

#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/mcrepo/generic_section.hpp"

namespace pstore {
    namespace exchange {

        //*  _  __ _                          _      *
        //* (_)/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
        //* | |  _| \ \ / || | '_ \ | '_| || | / -_) *
        //* |_|_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
        //*                  |_|                     *
        class ifixup_rule final : public rule {
        public:
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

        //*       __ _                          _      *
        //* __ __/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
        //* \ \ /  _| \ \ / || | '_ \ | '_| || | / -_) *
        //* /_\_\_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
        //*                    |_|                     *
        class xfixup_rule final : public rule {
        public:
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

        //*   __ _                        _     _        _    *
        //*  / _(_)_ ___  _ _ __ ___  ___| |__ (_)___ __| |_  *
        //* |  _| \ \ / || | '_ (_-< / _ \ '_ \| / -_) _|  _| *
        //* |_| |_/_\_\\_,_| .__/__/ \___/_.__// \___\__|\__| *
        //*                |_|               |__/             *
        //-MARK: fixups object
        template <typename Next, typename Fixup>
        class fixups_object final : public rule {
        public:
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

        using ifixups_object = fixups_object<ifixup_rule, repo::internal_fixup>;
        using xfixups_object = fixups_object<xfixup_rule, repo::external_fixup>;

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
