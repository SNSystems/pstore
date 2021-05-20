//===- include/pstore/exchange/import_generic_section.hpp -*- mode: C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP
#define PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP

#include "pstore/exchange/import_fixups.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/support/base64.hpp"

namespace pstore {
    namespace exchange {
        namespace import_ns {

            //*                        _                 _   _           *
            //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
            //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
            //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
            //* |___/                                                    *
            //-MARK: generic section
            template <typename OutputIterator>
            class generic_section : public rule {
            public:
                generic_section (not_null<context *> const ctxt, repo::section_kind const kind,
                                 not_null<string_mapping const *> const names,
                                 not_null<repo::section_content *> const content,
                                 not_null<OutputIterator *> const out) noexcept
                        : rule (ctxt)
                        , kind_{kind}
                        , names_{names}
                        , content_{content}
                        , out_{out} {}

                generic_section (generic_section const &) = delete;
                generic_section (generic_section &&) noexcept = delete;

                ~generic_section () noexcept override = default;

                generic_section & operator= (generic_section const &) = delete;
                generic_section & operator= (generic_section &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "generic section"; }
                std::error_code key (std::string const & k) override;
                std::error_code end_object () override;

            protected:
                error_or<repo::section_content *> content_object ();

            private:
                repo::section_kind const kind_;
                not_null<string_mapping const *> const names_;
                not_null<repo::section_content *> const content_;
                not_null<OutputIterator *> const out_;

                enum { align, data, ifixups, xfixups };
                std::bitset<xfixups + 1> seen_;

                std::string data_;
                std::uint64_t align_ = 1U;
            };

            // key
            // ~~~
            template <typename OutputIterator>
            std::error_code generic_section<OutputIterator>::key (std::string const & k) {
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
                    return push_array_rule<ifixups_object> (this, names_, &content_->ifixups);
                }
                if (k == "xfixups") {
                    seen_[xfixups] = true;
                    return push_array_rule<xfixups_object> (this, names_, &content_->xfixups);
                }
                return error::unrecognized_section_object_key;
            }

            // content object
            // ~~~~~~~~~~~~~~
            template <typename OutputIterator>
            error_or<repo::section_content *> generic_section<OutputIterator>::content_object () {
                using return_type = error_or<repo::section_content *>;

                // The alignment field may be omitted if it is 1.
                seen_[align] = true;
                // We allow either or both of the internal and external fixup keys to be omitted if
                // their respective contents are empty.
                seen_[ifixups] = true;
                seen_[xfixups] = true;

                // Issue an error is any of the required fields were missing.
                if (!seen_.all ()) {
                    return return_type{error::generic_section_was_incomplete};
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
                if (!from_base64 (std::begin (data_), std::end (data_),
                                  std::back_inserter (content_->data))) {
                    return return_type{error::bad_base64_data};
                }
                return return_type{content_};
            }

            // end object
            // ~~~~~~~~~~
            template <typename OutputIterator>
            std::error_code generic_section<OutputIterator>::end_object () {
                error_or<repo::section_content *> const c = this->content_object ();
                if (!c) {
                    return c.get_error ();
                }
                *out_ = std::make_unique<
                    repo::section_to_creation_dispatcher<repo::generic_section>::type> (kind_,
                                                                                        c.get ());
                return pop ();
            }

        } // end namespace import_ns
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_GENERIC_SECTION_HPP
