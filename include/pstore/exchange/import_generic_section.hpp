//*  _                            _                                _       *
//* (_)_ __ ___  _ __   ___  _ __| |_    __ _  ___ _ __   ___ _ __(_) ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _` |/ _ \ '_ \ / _ \ '__| |/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | (_| |  __/ | | |  __/ |  | | (__  *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__, |\___|_| |_|\___|_|  |_|\___| *
//*             |_|                     |___/                              *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===- include/pstore/exchange/import_generic_section.hpp -----------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP

#include "pstore/exchange/import_fixups.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/section.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {

        //*                        _                 _   _           *
        //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
        //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
        //* |___/                                                    *
        //-MARK: generic section
        template <typename TransactionLock, typename OutputIterator>
        class import_generic_section : public rule {
        public:
            using transaction_pointer = not_null<transaction<TransactionLock> *>;
            using names_pointer = not_null<import_name_mapping const *>;

            import_generic_section (parse_stack_pointer const stack, repo::section_kind kind,
                                    names_pointer const names,
                                    not_null<repo::section_content *> const content,
                                    not_null<OutputIterator *> const out) noexcept
                    : rule (stack)
                    , kind_{kind}
                    , names_{names}
                    , content_{content}
                    , out_{out} {}

            import_generic_section (import_generic_section const &) = delete;
            import_generic_section (import_generic_section &&) noexcept = delete;

            import_generic_section & operator= (import_generic_section const &) = delete;
            import_generic_section & operator= (import_generic_section &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "generic section"; }
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        protected:
            error_or<repo::section_content *> content_object ();

        private:
            repo::section_kind const kind_;
            names_pointer const names_;

            enum { align, data, ifixups, xfixups };
            std::bitset<xfixups + 1> seen_;

            std::string data_;
            std::uint64_t align_ = 1U;

            not_null<repo::section_content *> const content_;
            not_null<OutputIterator *> const out_;
        };

        // key
        // ~~~
        template <typename TransactionLock, typename OutputIterator>
        std::error_code
        import_generic_section<TransactionLock, OutputIterator>::key (std::string const & k) {
            if (k == "data") {
                seen_[data] = true; // string (ascii85)
                return this->push<string_rule> (&data_);
            }
            if (k == "align") {
                seen_[align] = true; // integer
                return this->push<uint64_rule> (&align_);
            }
            if (k == "ifixups") {
                seen_[ifixups] = true;
                return push_array_rule<ifixups_object<TransactionLock>> (this, names_,
                                                                         &content_->ifixups);
            }
            if (k == "xfixups") {
                seen_[xfixups] = true;
                return push_array_rule<xfixups_object<TransactionLock>> (this, names_,
                                                                         &content_->xfixups);
            }
            return import_error::unrecognized_section_object_key;
        }

        // content object
        // ~~~~~~~~~~~~~~
        template <typename TransactionLock, typename OutputIterator>
        error_or<repo::section_content *>
        import_generic_section<TransactionLock, OutputIterator>::content_object () {
            using return_type = error_or<repo::section_content *>;

            if (!seen_.all ()) {
                // FIXME: restore which check (but smarter)
                // return return_type{import_error::generic_section_was_incomplete};
            }
            if (!is_power_of_two (align_)) {
                return return_type{import_error::alignment_must_be_power_of_2};
            }
            using align_type = decltype (content_->align);
            static_assert (std::is_unsigned<align_type>::value,
                           "Expected alignment to be unsigned");
            if (align_ > std::numeric_limits<align_type>::max ()) {
                return return_type{import_error::alignment_is_too_great};
            }
            content_->kind = kind_;
            content_->align = static_cast<align_type> (align_);
            if (!from_base64 (std::begin (data_), std::end (data_),
                              std::back_inserter (content_->data))) {
                return return_type{import_error::bad_base64_data};
            }
            return return_type{content_};
        }

        // end object
        // ~~~~~~~~~~
        template <typename TransactionLock, typename OutputIterator>
        std::error_code import_generic_section<TransactionLock, OutputIterator>::end_object () {
            if (error_or<repo::section_content *> const c = this->content_object ()) {
                *out_ =
                    std::make_unique<repo::generic_section_creation_dispatcher> (kind_, c.get ());
                return pop ();
            } else {
                return c.get_error ();
            }
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP
