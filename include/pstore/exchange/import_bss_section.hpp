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
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP

#include <bitset>

#include "pstore/exchange/import_names.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/bss_section.hpp"

namespace pstore {
    namespace exchange {
        namespace import {

            template <typename OutputIterator>
            class bss_section : public rule {
            public:
                bss_section (not_null<context *> const ctxt, repo::section_kind const kind,
                             not_null<name_mapping const *> const /*names*/,
                             not_null<repo::section_content *> const content,
                             not_null<OutputIterator *> const out) noexcept
                        : rule (ctxt)
                        , kind_{kind}
                        , content_{content}
                        , out_{out} {}

                bss_section (bss_section const &) = delete;
                bss_section (bss_section &&) = delete;

                ~bss_section () noexcept override = default;

                bss_section & operator= (bss_section const &) = delete;
                bss_section & operator= (bss_section &&) = delete;

                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

                gsl::czstring name () const noexcept override { return "bss section"; }

            private:
                error_or<repo::section_content *> content_object ();

                repo::section_kind const kind_;
                not_null<repo::section_content *> const content_;
                not_null<OutputIterator *> const out_;

                enum { align, size };
                std::bitset<size + 1> seen_;

                std::uint64_t size_ = 0U;
                std::uint64_t align_ = 1U;
            };

            // key
            // ~~~
            template <typename OutputIterator>
            std::error_code bss_section<OutputIterator>::key (std::string const & k) {
                if (k == "align") {
                    seen_[align] = true; // integer
                    return this->push<uint64_rule> (&align_);
                }
                if (k == "size") {
                    seen_[size] = true;
                    return this->push<uint64_rule> (&size_);
                }
                return error::unrecognized_section_object_key;
            }

            template <typename OutputIterator>
            error_or<repo::section_content *> bss_section<OutputIterator>::content_object () {
                using return_type = error_or<repo::section_content *>;

                // The 'align' key may be omitted if the alignment is 1.
                seen_[align] = true;

                // Issue an error is any of the required fields were missing.
                if (!seen_.all ()) {
                    return return_type{error::bss_section_was_incomplete};
                }
                if (!is_power_of_two (align_)) {
                    return return_type{error::alignment_must_be_power_of_2};
                }
                using align_type = decltype (content_->align);
                static_assert (std::is_unsigned<align_type>::value,
                               "Expected alignment to be unsigned");
                if (align_ > std::numeric_limits<align_type>::max ()) {
                    return return_type{error::alignment_is_too_great};
                }
                content_->kind = kind_;
                content_->align = static_cast<align_type> (align_);
                content_->data.resize (size_);
                return return_type{content_};
            }

            // end object
            // ~~~~~~~~~~
            template <typename OutputIterator>
            std::error_code bss_section<OutputIterator>::end_object () {
                error_or<repo::section_content *> const c = this->content_object ();
                if (!c) {
                    return c.get_error ();
                }
                *out_ = std::make_unique<
                    repo::section_to_creation_dispatcher<repo::bss_section>::type> (c.get ());
                return pop ();
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_BSS_SECTION_HPP
