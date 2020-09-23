//*  _                            _      __ _                       *
//* (_)_ __ ___  _ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*             |_|                                     |_|         *
//===- include/pstore/exchange/import_fixups.hpp --------------------------===//
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
            class section_name final : public import_rule {
            public:
                section_name (parse_stack_pointer const stack,
                              not_null<pstore::repo::section_kind *> const section)
                        : import_rule (stack)
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
        //-MARK:import internal fixup
        class import_internal_fixup final : public import_rule {
        public:
            using names_pointer = not_null<import_name_mapping const *>;
            using fixups_pointer = not_null<std::vector<repo::internal_fixup> *>;

            import_internal_fixup (parse_stack_pointer stack, names_pointer names,
                                   fixups_pointer fixups);
            import_internal_fixup (import_internal_fixup const &) = delete;
            import_internal_fixup (import_internal_fixup &&) noexcept = delete;

            ~import_internal_fixup () noexcept override = default;

            import_internal_fixup & operator= (import_internal_fixup const &) = delete;
            import_internal_fixup & operator= (import_internal_fixup &&) noexcept = delete;

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
            std::uint64_t addend_ = 0;
        };


        //*          _                     _    __ _                *
        //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                  |_|    *
        //-MARK:import external fixup
        class import_external_fixup final : public import_rule {
        public:
            using names_pointer = not_null<import_name_mapping const *>;
            using fixups_pointer = not_null<std::vector<repo::external_fixup> *>;

            import_external_fixup (parse_stack_pointer stack, names_pointer names,
                                   fixups_pointer fixups);
            import_external_fixup (import_external_fixup const &) = delete;
            import_external_fixup (import_external_fixup &&) noexcept = delete;

            ~import_external_fixup () noexcept override = default;

            import_external_fixup & operator= (import_external_fixup const &) = delete;
            import_external_fixup & operator= (import_external_fixup &&) noexcept = delete;

            gsl::czstring name () const noexcept override;
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            enum { name_index, type, offset, addend };
            names_pointer const names_;
            std::bitset<addend + 1> seen_;
            fixups_pointer const fixups_;

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
        class fixups_object final : public import_rule {
        public:
            using names_pointer = not_null<import_name_mapping const *>;
            using fixups_pointer = not_null<std::vector<Fixup> *>;

            fixups_object (parse_stack_pointer const stack, names_pointer const names,
                           fixups_pointer const fixups)
                    : import_rule (stack)
                    , names_{names}
                    , fixups_{fixups} {}
            fixups_object (fixups_object const &) = delete;
            fixups_object (fixups_object &&) noexcept = delete;

            ~fixups_object () noexcept override = default;

            fixups_object & operator= (fixups_object const &) = delete;
            fixups_object & operator= (fixups_object &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "fixups object"; }
            std::error_code begin_object () override { return this->push<Next> (names_, fixups_); }
            std::error_code end_array () override { return pop (); }

        private:
            names_pointer const names_;
            fixups_pointer const fixups_;
        };

        using ifixups_object = fixups_object<import_internal_fixup, repo::internal_fixup>;
        using xfixups_object = fixups_object<import_external_fixup, repo::external_fixup>;

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_FIXUPS_HPP
