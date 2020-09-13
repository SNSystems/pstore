//*  _                            _     _              *
//* (_)_ __ ___  _ __   ___  _ __| |_  | |__  ___ ___  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \/ __/ __| *
//* | | | | | | | |_) | (_) | |  | |_  | |_) \__ \__ \ *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_.__/|___/___/ *
//*             |_|                                    *
//*                _   _              *
//*  ___  ___  ___| |_(_) ___  _ __   *
//* / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* \__ \  __/ (__| |_| | (_) | | | | *
//* |___/\___|\___|\__|_|\___/|_| |_| *
//*                                   *
//===- include/pstore/exchange/import_bss_section.hpp ---------------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP

#include <bitset>

#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/bss_section.hpp"

namespace pstore {
    namespace exchange {

        template <typename OutputIterator>
        class import_bss_section : public import_rule {
        public:
            using names_pointer = not_null<import_name_mapping const *>;
            using content_pointer = not_null<repo::section_content *>;

            import_bss_section (parse_stack_pointer const stack, repo::section_kind const kind,
                                not_null<database const *> /*db*/, names_pointer const /*names*/,
                                content_pointer const content,
                                not_null<OutputIterator *> const out) noexcept
                    : import_rule (stack)
                    , kind_{kind}
                    , content_{content}
                    , out_{out} {}

            import_bss_section (import_bss_section const &) = delete;
            import_bss_section (import_bss_section &&) = delete;

            import_bss_section & operator= (import_bss_section const &) = delete;
            import_bss_section & operator= (import_bss_section &&) = delete;

            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

            gsl::czstring name () const noexcept override { return "bss section"; }

        private:
            error_or<repo::section_content *> content_object ();

            repo::section_kind const kind_;
            content_pointer const content_;
            not_null<OutputIterator *> const out_;

            enum { align, size };
            std::bitset<size + 1> seen_;

            std::uint64_t size_ = 0U;
            std::uint64_t align_ = 1U;
        };

        // key
        // ~~~
        template <typename OutputIterator>
        std::error_code import_bss_section<OutputIterator>::key (std::string const & k) {
            if (k == "align") {
                seen_[align] = true; // integer
                return this->push<uint64_rule> (&align_);
            }
            if (k == "size") {
                seen_[size] = true;
                return this->push<uint64_rule> (&size_);
            }
            return import_error::unrecognized_section_object_key;
        }

        template <typename OutputIterator>
        error_or<repo::section_content *> import_bss_section<OutputIterator>::content_object () {
            using return_type = error_or<repo::section_content *>;

            // The 'align' key may be omitted if the alignment is 1.
            seen_[align] = true;

            // Issue an error is any of the required fields were missing.
            if (!seen_.all ()) {
                return return_type{import_error::bss_section_was_incomplete};
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
            content_->data.resize (size_);
            return return_type{content_};
        }

        // end object
        // ~~~~~~~~~~
        template <typename OutputIterator>
        std::error_code import_bss_section<OutputIterator>::end_object () {
            error_or<repo::section_content *> const c = this->content_object ();
            if (!c) {
                return c.get_error ();
            }
            *out_ =
                std::make_unique<repo::section_to_creation_dispatcher<repo::bss_section>::type> (
                    c.get ());
            return pop ();
        }

    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP
