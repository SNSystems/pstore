//*  _                            _    *
//* (_)_ __ ___  _ __   ___  _ __| |_  *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| *
//* | | | | | | | |_) | (_) | |  | |_  *
//* |_|_| |_| |_| .__/ \___/|_|   \__| *
//*             |_|                    *
//*   __                                      _    *
//*  / _|_ __ __ _  __ _ _ __ ___   ___ _ __ | |_  *
//* | |_| '__/ _` |/ _` | '_ ` _ \ / _ \ '_ \| __| *
//* |  _| | | (_| | (_| | | | | | |  __/ | | | |_  *
//* |_| |_|  \__,_|\__, |_| |_| |_|\___|_| |_|\__| *
//*                |___/                           *
//===- lib/exchange/import_fragment.cpp -----------------------------------===//
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
#include "pstore/exchange/import_fragment.hpp"

#include <bitset>
#include <unordered_map>
#include <vector>

#include "pstore/core/hamt_map.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/exchange/array_helpers.hpp"
#include "pstore/exchange/digest_from_string.hpp"
#include "pstore/exchange/import_error.hpp"
#include "pstore/exchange/import_non_terminals.hpp"
#include "pstore/exchange/import_rule.hpp"
#include "pstore/exchange/import_terminals.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "pstore/support/base64.hpp"

#include "import_fixups.hpp"

using pstore::exchange::import_error;
using pstore::exchange::names_pointer;
using pstore::exchange::rule;
using pstore::exchange::string_rule;
using pstore::exchange::transaction_pointer;
using pstore::exchange::uint64_rule;

namespace {

    //*                        _                 _   _           *
    //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
    //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
    //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
    //* |___/                                                    *
    //-MARK: generic section
    template <typename OutputIterator>
    class generic_section : public rule {
    public:
        generic_section (parse_stack_pointer const stack, pstore::repo::section_kind kind,
                         names_pointer const names,
                         pstore::gsl::not_null<pstore::repo::section_content *> const content,
                         pstore::gsl::not_null<OutputIterator *> const out) noexcept
                : rule (stack)
                , kind_{kind}
                , names_{names}
                , content_{content}
                , out_{out} {}

        generic_section (generic_section const &) = delete;
        generic_section (generic_section &&) noexcept = delete;

        generic_section & operator= (generic_section const &) = delete;
        generic_section & operator= (generic_section &&) noexcept = delete;

        pstore::gsl::czstring name () const noexcept override { return "generic section"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    protected:
        pstore::error_or<pstore::repo::section_content *> content_object ();

    private:
        pstore::repo::section_kind const kind_;
        names_pointer const names_;

        enum { align, data, ifixups, xfixups };
        std::bitset<xfixups + 1> seen_;

        std::string data_;
        std::uint64_t align_ = 1U;

        pstore::gsl::not_null<pstore::repo::section_content *> const content_;
        pstore::gsl::not_null<OutputIterator *> const out_;
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
            return pstore::exchange::push_array_rule<pstore::exchange::ifixups_object> (
                this, names_, &content_->ifixups);
        }
        if (k == "xfixups") {
            seen_[xfixups] = true;
            return pstore::exchange::push_array_rule<pstore::exchange::xfixups_object> (
                this, names_, &content_->xfixups);
        }
        return import_error::unrecognized_section_object_key;
    }

    // content object
    // ~~~~~~~~~~~~~~
    template <typename OutputIterator>
    pstore::error_or<pstore::repo::section_content *>
    generic_section<OutputIterator>::content_object () {
        using return_type = pstore::error_or<pstore::repo::section_content *>;

        if (!seen_.all ()) {
            // FIXME: restore which check (but smarter)
            // return return_type{import_error::generic_section_was_incomplete};
        }
        if (!pstore::is_power_of_two (align_)) {
            return return_type{import_error::alignment_must_be_power_of_2};
        }
        using align_type = decltype (content_->align);
        static_assert (std::is_unsigned<align_type>::value, "Expected alignment to be unsigned");
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
    template <typename OutputIterator>
    std::error_code generic_section<OutputIterator>::end_object () {
        if (pstore::error_or<pstore::repo::section_content *> const c = this->content_object ()) {
            *out_ = std::make_unique<pstore::repo::generic_section_creation_dispatcher> (kind_,
                                                                                         c.get ());
            return pop ();
        } else {
            return c.get_error ();
        }
    }


    //*     _     _                _ _                       _   _           *
    //*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
    //* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
    //* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
    //*                    |___/                                             *
    //-MARK: debug line section
    template <typename OutputIterator>
    class debug_line_section final : public generic_section<OutputIterator> {
    public:
        debug_line_section (rule::parse_stack_pointer const stack, pstore::database & db,
                            names_pointer const names,
                            pstore::repo::section_content * const content,
                            OutputIterator * const out)
                : generic_section<OutputIterator> (stack, pstore::repo::section_kind::debug_line,
                                                   names, content, out)
                , db_{db}
                , out_{out} {}

        pstore::gsl::czstring name () const noexcept override { return "debug line section"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        enum { header };
        std::bitset<header + 1> seen_;

        pstore::database & db_;
        std::string header_digest_;
        pstore::gsl::not_null<OutputIterator *> const out_;
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
        pstore::maybe<pstore::index::digest> const digest =
            pstore::exchange::digest_from_string (header_digest_);
        if (!digest) {
            return import_error::bad_digest;
        }
        if (!seen_.all ()) {
            return import_error::incomplete_debug_line_section;
        }

        auto const index =
            pstore::index::get_index<pstore::trailer::indices::debug_line_header> (db_);
        auto pos = index->find (db_, *digest);
        if (pos == index->end (db_)) {
            return import_error::debug_line_header_digest_not_found;
        }
        auto const header_extent = pos->second;

        if (pstore::error_or<pstore::repo::section_content *> const content =
                this->content_object ()) {
            *out_ = std::make_unique<pstore::repo::debug_line_section_creation_dispatcher> (
                *digest, header_extent, content.get ());
            return this->pop ();
        } else {
            return content.get_error ();
        }
    }


    //*   __                             _                _   _              *
    //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
    //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
    //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
    //*              |___/                                                   *
    //-MARK: fragment sections
    class fragment_sections final : public rule {
    public:
        fragment_sections (parse_stack_pointer const stack, transaction_pointer const transaction,
                           names_pointer const names, pstore::index::digest const * const digest)
                : rule (stack)
                , transaction_{transaction}
                , names_{names}
                , digest_{digest}
                , oit_{dispatchers_} {}
        pstore::gsl::czstring name () const noexcept override { return "fragment sections"; }
        std::error_code key (std::string const & s) override;
        std::error_code end_object () override;

    private:
        transaction_pointer const transaction_;
        names_pointer const names_;
        pstore::index::digest const * const digest_;

        pstore::repo::section_content *
        section_contents (pstore::repo::section_kind kind) noexcept {
            return &contents_[static_cast<std::underlying_type<pstore::repo::section_kind>::type> (
                kind)];
        }

        std::array<pstore::repo::section_content, pstore::repo::num_section_kinds> contents_;
        std::vector<std::unique_ptr<pstore::repo::section_creation_dispatcher>> dispatchers_;
        std::back_insert_iterator<decltype (dispatchers_)> oit_;
    };

    // key
    // ~~~
    std::error_code fragment_sections::key (std::string const & s) {
        using pstore::repo::section_kind;

#define X(a) {#a, section_kind::a},
        static std::unordered_map<std::string, section_kind> const map{PSTORE_MCREPO_SECTION_KINDS};
#undef X
        auto const pos = map.find (s);
        if (pos == map.end ()) {
            return import_error::unknown_section_name;
        }

        switch (pos->second) {
        case section_kind::data:
        case section_kind::interp:
        case section_kind::mergeable_1_byte_c_string:
        case section_kind::mergeable_2_byte_c_string:
        case section_kind::mergeable_4_byte_c_string:
        case section_kind::mergeable_const_16:
        case section_kind::mergeable_const_32:
        case section_kind::mergeable_const_4:
        case section_kind::mergeable_const_8:
        case section_kind::read_only:
        case section_kind::rel_ro:
        case section_kind::text:
        case section_kind::thread_data:
            return pstore::exchange::push_object_rule<generic_section<decltype (oit_)>> (
                this, pos->second, names_, section_contents (pos->second), &oit_);

        case section_kind::debug_line:
            return pstore::exchange::push_object_rule<debug_line_section<decltype (oit_)>> (
                this, transaction_->db (), names_, section_contents (section_kind::debug_line),
                &oit_);

        case section_kind::last: assert (false && "Illegal section kind"); // unreachable
        case section_kind::bss:
        case section_kind::thread_bss:
        case section_kind::debug_string:
        case section_kind::debug_ranges:
        case section_kind::dependent: assert (false && "Unimplemented section kind"); // unreachable
        }
        return import_error::unknown_section_name;
    }

    // end object
    // ~~~~~~~~~~
    std::error_code fragment_sections::end_object () {
        std::shared_ptr<pstore::index::fragment_index> const index =
            pstore::index::get_index<pstore::trailer::indices::fragment> (transaction_->db (),
                                                                          true /* create */);
        index->insert (
            *transaction_,
            std::make_pair (*digest_,
                            pstore::repo::fragment::alloc (
                                *transaction_, pstore::make_pointee_adaptor (dispatchers_.begin ()),
                                pstore::make_pointee_adaptor (dispatchers_.end ()))));
        return pop ();
    }

} // end anonymous namespace

namespace pstore {
    namespace exchange {

        //*   __                             _     _         _          *
        //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  (_)_ _  __| |_____ __ *
        //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| | | ' \/ _` / -_) \ / *
        //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| |_|_||_\__,_\___/_\_\ *
        //*              |___/                                          *
        //-MARK: fragment index
        // (ctor)
        // ~~~~~~
        fragment_index::fragment_index (parse_stack_pointer const stack,
                                        transaction_pointer const transaction,
                                        names_pointer const names)
                : rule (stack)
                , transaction_{transaction}
                , names_{names} {
            sections_.reserve (
                static_cast<std::underlying_type_t<repo::section_kind>> (repo::section_kind::last));
        }

        // name
        // ~~~~
        gsl::czstring fragment_index::name () const noexcept { return "fragment index"; }

        // key
        // ~~~
        std::error_code fragment_index::key (std::string const & s) {
            if (maybe<index::digest> const digest = digest_from_string (s)) {
                digest_ = *digest;
                return push_object_rule<fragment_sections> (this, transaction_, names_, &digest_);
            }
            return import_error::bad_digest;
        }

        // end object
        // ~~~~~~~~~~
        std::error_code fragment_index::end_object () { return pop (); }

    } // end namespace exchange
} // end namespace pstore
