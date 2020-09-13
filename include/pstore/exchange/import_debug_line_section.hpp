//*  _                            _         _      _                  *
//* (_)_ __ ___  _ __   ___  _ __| |_    __| | ___| |__  _   _  __ _  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __|  / _` |/ _ \ '_ \| | | |/ _` | *
//* | | | | | | | |_) | (_) | |  | |_  | (_| |  __/ |_) | |_| | (_| | *
//* |_|_| |_| |_| .__/ \___/|_|   \__|  \__,_|\___|_.__/ \__,_|\__, | *
//*             |_|                                            |___/  *
//*  _ _                            _   _              *
//* | (_)_ __   ___   ___  ___  ___| |_(_) ___  _ __   *
//* | | | '_ \ / _ \ / __|/ _ \/ __| __| |/ _ \| '_ \  *
//* | | | | | |  __/ \__ \  __/ (__| |_| | (_) | | | | *
//* |_|_|_| |_|\___| |___/\___|\___|\__|_|\___/|_| |_| *
//*                                                    *
//===- include/pstore/exchange/import_debug_line_section.hpp --------------===//
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
#ifndef PSTORE_EXCHANGE_IMPORT_DEBUG_LIINE_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_DEBUG_LIINE_SECTION_HPP

#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/mcrepo/debug_line_section.hpp"

namespace pstore {
    namespace exchange {

        //*     _     _                _ _                       _   _           *
        //*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
        //* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
        //* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
        //*                    |___/                                             *
        //-MARK: debug line section
        template <typename OutputIterator>
        class import_debug_line_section final : public import_generic_section<OutputIterator> {
        public:
            using names_pointer = not_null<import_name_mapping const *>;

            import_debug_line_section (import_rule::parse_stack_pointer const stack,
                                       repo::section_kind kind, database & db,
                                       names_pointer const names,
                                       repo::section_content * const content,
                                       OutputIterator * const out)
                    : import_generic_section<OutputIterator> (stack, kind, db, names, content, out)
                    , db_{db}
                    , out_{out} {
                assert (kind == repo::section_kind::debug_line);
            }
            import_debug_line_section (import_debug_line_section const &) = delete;
            import_debug_line_section (import_debug_line_section &&) noexcept = delete;

            import_debug_line_section & operator= (import_debug_line_section const &) = delete;
            import_debug_line_section & operator= (import_debug_line_section &&) noexcept = delete;

            gsl::czstring name () const noexcept override { return "debug line section"; }
            std::error_code key (std::string const & k) override;
            std::error_code end_object () override;

        private:
            enum { header };
            std::bitset<header + 1> seen_;

            database & db_;
            std::string header_digest_;
            not_null<OutputIterator *> const out_;
        };

        // key
        // ~~~
        template <typename OutputIterator>
        std::error_code import_debug_line_section<OutputIterator>::key (std::string const & k) {
            if (k == "header") {
                seen_[header] = true;
                return this->template push<string_rule> (&header_digest_);
            }
            return import_generic_section<OutputIterator>::key (k);
        }

        // end object
        // ~~~~~~~~~~
        template <typename OutputIterator>
        std::error_code import_debug_line_section<OutputIterator>::end_object () {
            maybe<index::digest> const digest = exchange::digest_from_string (header_digest_);
            if (!digest) {
                return import_error::bad_digest;
            }
            if (!seen_.all ()) {
                return import_error::incomplete_debug_line_section;
            }

            auto const index = index::get_index<trailer::indices::debug_line_header> (db_);
            auto pos = index->find (db_, *digest);
            if (pos == index->end (db_)) {
                return import_error::debug_line_header_digest_not_found;
            }
            auto const header_extent = pos->second;

            if (error_or<repo::section_content *> const content = this->content_object ()) {
                *out_ = std::make_unique<repo::debug_line_section_creation_dispatcher> (
                    *digest, header_extent, content.get ());
                return this->pop ();
            } else {
                return content.get_error ();
            }
        }



    } // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_DEBUG_LIINE_SECTION_HPP
