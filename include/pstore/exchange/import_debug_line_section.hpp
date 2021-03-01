//===- include/pstore/exchange/import_debug_line_section.hpp *- mode: C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_SECTION_HPP

#include "pstore/exchange/import_generic_section.hpp"
#include "pstore/mcrepo/debug_line_section.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            //*     _     _                _ _                       _   _           *
            //*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
            //* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
            //* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
            //*                    |___/                                             *
            //-MARK: debug line section
            template <typename OutputIterator>
            class debug_line_section final : public generic_section<OutputIterator> {
            public:
                debug_line_section (not_null<context *> const ctxt, repo::section_kind kind,
                                    not_null<name_mapping const *> const names,
                                    repo::section_content * const content,
                                    OutputIterator * const out)
                        : generic_section<OutputIterator> (ctxt, kind, names, content, out)
                        , out_{out} {
                    PSTORE_ASSERT (kind == repo::section_kind::debug_line);
                }
                debug_line_section (debug_line_section const &) = delete;
                debug_line_section (debug_line_section &&) noexcept = delete;

                ~debug_line_section () noexcept override = default;

                debug_line_section & operator= (debug_line_section const &) = delete;
                debug_line_section & operator= (debug_line_section &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "debug line section"; }
                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            private:
                enum { header };
                std::bitset<header + 1> seen_;

                std::string header_digest_;
                not_null<OutputIterator *> const out_;
            };

            // key
            // ~~~
            template <typename OutputIterator>
            std::error_code debug_line_section<OutputIterator>::key (std::string const & k) {
                if (k == "header") {
                    seen_[header] = true;
                    return this->template push<string_rule> (&header_digest_);
                }
                return generic_section<OutputIterator>::key (k);
            }

            // end object
            // ~~~~~~~~~~
            template <typename OutputIterator>
            std::error_code debug_line_section<OutputIterator>::end_object () {
                maybe<index::digest> const digest = uint128::from_hex_string (header_digest_);
                if (!digest) {
                    return error::bad_digest;
                }
                if (!seen_.all ()) {
                    return error::incomplete_debug_line_section;
                }

                database * const db = this->get_context ()->db;
                auto const index = index::get_index<trailer::indices::debug_line_header> (*db);
                auto pos = index->find (*db, *digest);
                if (pos == index->end (*db)) {
                    return error::debug_line_header_digest_not_found;
                }
                auto const header_extent = pos->second;

                error_or<repo::section_content *> const content = this->content_object ();
                if (!content) {
                    return content.get_error ();
                }

                *out_ = std::make_unique<repo::debug_line_section_creation_dispatcher> (
                    *digest, header_extent, content.get ());
                return this->pop ();
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_DEBUG_LINE_SECTION_HPP
