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
#include "import_fixups.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_terminals.hpp"

namespace {

    //*             _   _                                 *
    //*  ___ ___ __| |_(_)___ _ _    _ _  __ _ _ __  ___  *
    //* (_-</ -_) _|  _| / _ \ ' \  | ' \/ _` | '  \/ -_) *
    //* /__/\___\__|\__|_\___/_||_| |_||_\__,_|_|_|_\___| *
    //*                                                   *
    //-MARK: section name
    class section_name final : public pstore::exchange::rule {
    public:
        section_name (parse_stack_pointer const stack,
                      pstore::gsl::not_null<pstore::repo::section_kind *> section)
                : rule (stack)
                , section_{section} {}

        pstore::gsl::czstring name () const noexcept override { return "section name"; }
        std::error_code string_value (std::string const & s) override;

    private:
        pstore::gsl::not_null<pstore::repo::section_kind *> section_;
    };

    // string value
    // ~~~~~~~~~~~~
    std::error_code section_name::string_value (std::string const & s) {
        // TODO: this map appears both here and in the fragment code.
#define X(a) {#a, pstore::repo::section_kind::a},
        static std::unordered_map<std::string, pstore::repo::section_kind> map = {
            PSTORE_MCREPO_SECTION_KINDS};
#undef X
        auto pos = map.find (s);
        if (pos == map.end ()) {
            return pstore::exchange::import_error::unknown_section_name;
        }
        *section_ = pos->second;
        return pop ();
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        //*  _  __ _                          _      *
        //* (_)/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
        //* | |  _| \ \ / || | '_ \ | '_| || | / -_) *
        //* |_|_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
        //*                  |_|                     *
        ifixup_rule::ifixup_rule (parse_stack_pointer const stack, names_pointer const /*names*/,
                                  std::vector<pstore::repo::internal_fixup> * fixups)
                : rule (stack)
                , fixups_{fixups} {}

        // key
        // ~~~
        std::error_code ifixup_rule::key (std::string const & k) {
            if (k == "section") {
                seen_[section] = true;
                return this->push<section_name> (&section_);
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
        std::error_code ifixup_rule::end_object () {
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
        // key
        // ~~~
        std::error_code xfixup_rule::key (std::string const & k) {
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
        std::error_code xfixup_rule::end_object () {
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

    } // end namespace exchange
} // end namespace pstore
