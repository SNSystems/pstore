//*  _                            _      __ _                       *
//* (_)_ __ ___  _ __   ___  _ __| |_   / _(_)_  ___   _ _ __  ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | |_| \ \/ / | | | '_ \/ __| *
//* | | | | | | | |_) | (_) | |  | |_  |  _| |>  <| |_| | |_) \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_/_/\_\\__,_| .__/|___/ *
//*             |_|                                     |_|         *
//===- lib/exchange/import_fixups.cpp -------------------------------------===//
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
#include "pstore/exchange/import_fixups.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            namespace details {

                // string value
                // ~~~~~~~~~~~~
                std::error_code section_name::string_value (std::string const & s) {
                    // TODO: this map appears both here and in the fragment code.
#define X(a) {#a, pstore::repo::section_kind::a},
                static std::unordered_map<std::string, pstore::repo::section_kind> map = {
                    PSTORE_MCREPO_SECTION_KINDS};
#undef X
                auto const pos = map.find (s);
                if (pos == map.end ()) {
                    return import::error::unknown_section_name;
                }
                *section_ = pos->second;
                return pop ();
            }

        } // end namespace details

        //*  _     _                     _    __ _                *
        //* (_)_ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* | | ' \  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* |_|_||_\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                |_|    *
        //-MARK:internal fixup
        // (ctor)
        // ~~~~~~
        internal_fixup::internal_fixup (not_null<context *> const ctxt,
                                        not_null<name_mapping const *> const /*names*/,
                                        fixups_pointer const fixups)
                : rule (ctxt)
                , fixups_{fixups} {}

        // name
        // ~~~~
        gsl::czstring internal_fixup::name () const noexcept { return "internal fixup"; }

        // key
        // ~~~
        std::error_code internal_fixup::key (std::string const & k) {
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
            return error::unrecognized_ifixup_key;
        }

        // end object
        // ~~~~~~~~~~
        std::error_code internal_fixup::end_object () {
            if (!seen_.all ()) {
                return error::ifixup_object_was_incomplete;
            }
            // TODO: validate more values here.
            fixups_->emplace_back (section_, static_cast<pstore::repo::relocation_type> (type_),
                                   offset_, addend_);
            return pop ();
        }

        //*          _                     _    __ _                *
        //*  _____ _| |_ ___ _ _ _ _  __ _| |  / _(_)_ ___  _ _ __  *
        //* / -_) \ /  _/ -_) '_| ' \/ _` | | |  _| \ \ / || | '_ \ *
        //* \___/_\_\\__\___|_| |_||_\__,_|_| |_| |_/_\_\\_,_| .__/ *
        //*                                                  |_|    *
        //-MARK:external fixup
        // (ctor)
        // ~~~~~~
        external_fixup::external_fixup (not_null<context *> const ctxt,
                                        not_null<name_mapping const *> const names,
                                        fixups_pointer const fixups)
                : rule (ctxt)
                , names_{names}
                , fixups_{fixups} {}

        // name
        // ~~~~~
        gsl::czstring external_fixup::name () const noexcept { return "external fixup"; }

        // key
        // ~~~
        std::error_code external_fixup::key (std::string const & k) {
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
            return error::unrecognized_xfixup_key;
        }

        // end object
        // ~~~~~~~~~~
        std::error_code external_fixup::end_object () {
            if (!seen_.all ()) {
                return error::xfixup_object_was_incomplete;
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

        } // end namespace import
    } // end namespace exchange
} // end namespace pstore
