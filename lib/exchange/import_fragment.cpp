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

using namespace pstore;
using namespace pstore::exchange;

namespace {

    //*             _   _                                 *
    //*  ___ ___ __| |_(_)___ _ _    _ _  __ _ _ __  ___  *
    //* (_-</ -_) _|  _| / _ \ ' \  | ' \/ _` | '  \/ -_) *
    //* /__/\___\__|\__|_\___/_||_| |_||_\__,_|_|_|_\___| *
    //*                                                   *
    //-MARK: section name
    class section_name final : public rule {
    public:
        section_name (parse_stack_pointer s, gsl::not_null<repo::section_kind *> section)
                : rule (s)
                , section_{section} {}

        gsl::czstring name () const noexcept override { return "section name"; }
        std::error_code string_value (std::string const & s) override;

    private:
        gsl::not_null<repo::section_kind *> section_;
    };

    // string value
    // ~~~~~~~~~~~~
    std::error_code section_name::string_value (std::string const & s) {
        // TODO: this map appears both here and in the fragment code.
#define X(a) {#a, repo::section_kind::a},
        static std::unordered_map<std::string, repo::section_kind> map = {
            PSTORE_MCREPO_SECTION_KINDS};
#undef X
        auto pos = map.find (s);
        if (pos == map.end ()) {
            return import_error::unknown_section_name;
        }
        *section_ = pos->second;
        return pop ();
    }


    //*  _  __ _                          _      *
    //* (_)/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
    //* | |  _| \ \ / || | '_ \ | '_| || | / -_) *
    //* |_|_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
    //*                  |_|                     *
    //-MARK: ifixup rule
    class ifixup_rule final : public rule {
    public:
        explicit ifixup_rule (parse_stack_pointer s, std::vector<repo::internal_fixup> * fixups)
                : rule (s)
                , fixups_{fixups} {}

        gsl::czstring name () const noexcept override { return "ifixup rule"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        enum { section, type, offset, addend };
        std::bitset<addend + 1> seen_;

        gsl::not_null<std::vector<repo::internal_fixup> *> fixups_;

        repo::section_kind section_ = repo::section_kind::last;
        std::uint64_t type_ = 0;
        std::uint64_t offset_ = 0;
        std::uint64_t addend_ = 0;
    };

    // key
    // ~~~
    std::error_code ifixup_rule::key (std::string const & k) {
        if (k == "section") {
            seen_[section] = true;
            return push<section_name> (&section_);
        }
        if (k == "type") {
            seen_[type] = true;
            return push<uint64_rule> (&type_);
        }
        if (k == "offset") {
            seen_[offset] = true;
            return push<uint64_rule> (&offset_);
        }
        if (k == "addend") {
            seen_[addend] = true;
            return push<uint64_rule> (&addend_);
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
        fixups_->emplace_back (section_, static_cast<repo::relocation_type> (type_), offset_,
                               addend_);
        return pop ();
    }


    //*       __ _                          _      *
    //* __ __/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
    //* \ \ /  _| \ \ / || | '_ \ | '_| || | / -_) *
    //* /_\_\_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
    //*                    |_|                     *
    //-MARK:xfixup rule
    class xfixup_rule final : public rule {
    public:
        xfixup_rule (parse_stack_pointer s, not_null<std::vector<repo::external_fixup> *> fixups)
                : rule (s)
                , fixups_{fixups} {}

        gsl::czstring name () const noexcept override { return "xfixup rule"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        enum { name_index, type, offset, addend };
        std::bitset<addend + 1> seen_;
        gsl::not_null<std::vector<repo::external_fixup> *> fixups_;

        std::uint64_t name_ = 0;
        std::uint64_t type_ = 0;
        std::uint64_t offset_ = 0;
        std::uint64_t addend_ = 0;
    };

    // key
    // ~~~
    std::error_code xfixup_rule::key (std::string const & k) {
        if (k == "name") {
            seen_[name_index] = true;
            return push<uint64_rule> (&name_);
        }
        if (k == "type") {
            seen_[type] = true;
            return push<uint64_rule> (&type_);
        }
        if (k == "offset") {
            seen_[offset] = true;
            return push<uint64_rule> (&offset_);
        }
        if (k == "addend") {
            seen_[addend] = true;
            return push<uint64_rule> (&addend_);
        }
        return import_error::unrecognized_xfixup_key;
    }

    // end object
    // ~~~~~~~~~~
    std::error_code xfixup_rule::end_object () {
        if (!seen_.all ()) {
            return import_error::xfixup_object_was_incomplete;
        }
        // TODO: validate some values here.
        fixups_->emplace_back (typed_address<indirect_string>::make (name_),
                               static_cast<repo::relocation_type> (type_), offset_, addend_);
        return pop ();
    }


    //*   __ _                        _     _        _    *
    //*  / _(_)_ ___  _ _ __ ___  ___| |__ (_)___ __| |_  *
    //* |  _| \ \ / || | '_ (_-< / _ \ '_ \| / -_) _|  _| *
    //* |_| |_/_\_\\_,_| .__/__/ \___/_.__// \___\__|\__| *
    //*                |_|               |__/             *
    //-MARK: fixups object
    template <typename Next, typename Fixup>
    class fixups_object final : public rule {
    public:
        fixups_object (parse_stack_pointer stack, gsl::not_null<std::vector<Fixup> *> fixups)
                : rule (stack)
                , fixups_{fixups} {}
        gsl::czstring name () const noexcept override { return "fixups object"; }
        std::error_code begin_object () override { return push<Next> (fixups_); }
        std::error_code end_array () override { return pop (); }

    private:
        gsl::not_null<std::vector<Fixup> *> fixups_;
    };

    using ifixups_object = fixups_object<ifixup_rule, repo::internal_fixup>;
    using xfixups_object = fixups_object<xfixup_rule, repo::external_fixup>;


    //*                        _                 _   _           *
    //*  __ _ ___ _ _  ___ _ _(_)__   ___ ___ __| |_(_)___ _ _   *
    //* / _` / -_) ' \/ -_) '_| / _| (_-</ -_) _|  _| / _ \ ' \  *
    //* \__, \___|_||_\___|_| |_\__| /__/\___\__|\__|_\___/_||_| *
    //* |___/                                                    *
    //-MARK: generic section
    template <typename OutputIterator>
    class generic_section final : public rule {
    public:
        generic_section (parse_stack_pointer stack, repo::section_kind kind,
                         not_null<repo::section_content *> const content,
                         not_null<OutputIterator *> const out) noexcept
                : rule (stack)
                , kind_{kind}
                , content_{content}
                , out_{out} {}
        gsl::czstring name () const noexcept override { return "generic section"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        repo::section_kind const kind_;

        enum { align, data, ifixups, xfixups };
        std::bitset<xfixups + 1> seen_;

        std::string data_;
        std::uint64_t align_ = 0;

        not_null<repo::section_content *> const content_;
        not_null<OutputIterator *> const out_;
    };

    // key
    // ~~~
    template <typename OutputIterator>
    std::error_code generic_section<OutputIterator>::key (std::string const & k) {
        if (k == "data") {
            seen_[data] = true; // string (ascii85)
            return push<string_rule> (&data_);
        }
        if (k == "align") {
            seen_[align] = true; // integer
            return this->push<uint64_rule> (&align_);
        }
        if (k == "ifixups") {
            seen_[ifixups] = true;
            return push_array_rule<ifixups_object> (this, &content_->ifixups);
        }
        if (k == "xfixups") {
            seen_[xfixups] = true;
            return push_array_rule<xfixups_object> (this, &content_->xfixups);
        }
        return import_error::unrecognized_section_object_key;
    }

    // end object
    // ~~~~~~~~~~
    template <typename OutputIterator>
    std::error_code generic_section<OutputIterator>::end_object () {
        if (!seen_.all ()) {
            return import_error::generic_section_was_incomplete;
        }
        if (!is_power_of_two (align_)) {
            return import_error::alignment_must_be_power_of_2;
        }
        using align_type = decltype (content_->align);
        static_assert (std::is_unsigned<align_type>::value, "Expected alignment to be unsigned");
        if (align_ > std::numeric_limits<align_type>::max ()) {
            return import_error::alignment_is_too_great;
        }
        content_->align = static_cast<align_type> (align_);
        if (!from_base64 (std::begin (data_), std::end (data_),
                          std::back_inserter (content_->data))) {
            return import_error::bad_base64_data;
        }

        *out_ =
            std::make_unique<repo::generic_section_creation_dispatcher> (kind_, content_.get ());
        return pop ();
    }


    //*     _     _                _ _                       _   _           *
    //*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
    //* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
    //* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
    //*                    |___/                                             *
    //-MARK: debug line section
    template <typename OutputIterator>
    class debug_line_section final : public rule {
    public:
        debug_line_section (parse_stack_pointer stack, database & db,
                            repo::section_content * const content, OutputIterator * const out)
                : rule (stack)
                , db_{db}
                , content_{content}
                , out_{out} {}

        gsl::czstring name () const noexcept override { return "debug line section"; }
        std::error_code key (std::string const & k) override;
        std::error_code end_object () override;

    private:
        enum { header, data, ifixups };
        std::bitset<ifixups + 1> seen_;

        database & db_;
        repo::section_content * const content_;
        OutputIterator * const out_;

        std::string header_digest_;
        std::string data_;
    };

    // key
    // ~~~
    template <typename OutputIterator>
    std::error_code debug_line_section<OutputIterator>::key (std::string const & k) {
        if (k == "header") {
            seen_[header] = true;
            return push<string_rule> (&header_digest_);
        }
        if (k == "data") {
            seen_[data] = true; // string (ascii85)
            return push<string_rule> (&data_);
        }
        if (k == "ifixups") {
            seen_[ifixups] = true;
            return push_array_rule<ifixups_object> (this, &content_->ifixups);
        }
        return import_error::unrecognized_section_object_key;
    }

    // end object
    // ~~~~~~~~~~
    template <typename OutputIterator>
    std::error_code debug_line_section<OutputIterator>::end_object () {
        maybe<index::digest> const digest = digest_from_string (header_digest_);
        if (!digest) {
            return import_error::bad_digest;
        }
        if (!seen_.all ()) {
            return import_error::incomplete_debug_line_section;
        }

        auto index = index::get_index<trailer::indices::debug_line_header> (db_);
        // FIXME: search the index for the DLH associated with 'digest'. Record the hash and its
        // associated extent in the debug_line_section instance.
        auto pos = index->find (db_, *digest);
        if (pos == index->end (db_)) {
            return import_error::debug_line_header_digest_not_found;
        }
        auto const header_extent = pos->second;


        repo::section_content content{repo::section_kind::debug_line, std::uint8_t{1} /*align*/};
        if (!from_base64 (std::begin (data_), std::end (data_),
                          std::back_inserter (content.data))) {
            return import_error::bad_base64_data;
        }
        // content.xfixups =
        // content.ifixups = std::move (ifixups_);
        //*content_ = just<debug_line_content> (in_place, std::move (content), header_extent);
        /// content_->emplace (std::move (content));
        *out_ = std::make_unique<repo::debug_line_section_creation_dispatcher> (header_extent,
                                                                                content_);
        return pop ();
    }


    //*   __                             _                _   _              *
    //*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
    //* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
    //* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
    //*              |___/                                                   *
    //-MARK: fragment sections
    class fragment_sections final : public rule {
    public:
        fragment_sections (parse_stack_pointer s, transaction_pointer transaction)
                : rule (s)
                , transaction_{transaction}
                //, debug_line_content_{repo::section_kind::debug_line}
                //, m1_content_{repo::section_kind::mergeable_1_byte_c_string},
                , oit_{sections_} {}
        gsl::czstring name () const noexcept override { return "fragment sections"; }
        std::error_code key (std::string const & s) override;
        std::error_code end_object () override { return pop (); }

    private:
        transaction_pointer transaction_;

        repo::section_content * section_contents (repo::section_kind kind) noexcept {
            return &contents_[static_cast<std::underlying_type<repo::section_kind>::type> (kind)];
        }

        std::array<repo::section_content, repo::num_section_kinds> contents_;
        std::vector<std::unique_ptr<repo::section_creation_dispatcher>> sections_;
        std::back_insert_iterator<decltype (sections_)> oit_;
    };

    // key
    // ~~~
    std::error_code fragment_sections::key (std::string const & s) {
#define X(a) {#a, repo::section_kind::a},
        static std::unordered_map<std::string, repo::section_kind> const map{
            PSTORE_MCREPO_SECTION_KINDS};
#undef X
        auto const pos = map.find (s);
        if (pos == map.end ()) {
            return import_error::unknown_section_name;
        }

        switch (pos->second) {
        case repo::section_kind::data:
        case repo::section_kind::interp:
        case repo::section_kind::mergeable_1_byte_c_string:
        case repo::section_kind::mergeable_2_byte_c_string:
        case repo::section_kind::mergeable_4_byte_c_string:
        case repo::section_kind::mergeable_const_16:
        case repo::section_kind::mergeable_const_32:
        case repo::section_kind::mergeable_const_4:
        case repo::section_kind::mergeable_const_8:
        case repo::section_kind::read_only:
        case repo::section_kind::rel_ro:
        case repo::section_kind::text:
        case repo::section_kind::thread_data:
            return push_object_rule<generic_section<decltype (oit_)>> (
                this, pos->second, section_contents (pos->second), &oit_);

        case repo::section_kind::debug_line:
            return push_object_rule<debug_line_section<decltype (oit_)>> (
                this, transaction_->db (), section_contents (repo::section_kind::debug_line),
                &oit_);

        case repo::section_kind::last: assert (false && "Illegal section kind"); // unreachable
        case repo::section_kind::bss:
        case repo::section_kind::thread_bss:
        case repo::section_kind::debug_string:
        case repo::section_kind::debug_ranges:
        case repo::section_kind::dependent:
            assert (false && "Unimplemented section kind"); // unreachable
        }
        return import_error::unknown_section_name;
    }

} // end anonymous namespace


//*   __                             _     _         _          *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  (_)_ _  __| |_____ __ *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| | | ' \/ _` / -_) \ / *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| |_|_||_\__,_\___/_\_\ *
//*              |___/                                          *
//-MARK: fragment index
// (ctor)
// ~~~~~~
fragment_index::fragment_index (parse_stack_pointer s, transaction_pointer transaction)
        : rule (s)
        , transaction_{transaction} {
    sections_.reserve (
        static_cast<std::underlying_type_t<repo::section_kind>> (repo::section_kind::last));
}

// name
// ~~~~
gsl::czstring fragment_index::name () const noexcept {
    return "fragment index";
}

// key
// ~~~
std::error_code fragment_index::key (std::string const & s) {
    if (maybe<uint128> const digest = digest_from_string (s)) {
        digest_ = *digest;
        return push_object_rule<fragment_sections> (this, transaction_);
    }
    return import_error::bad_digest;
}

// end object
// ~~~~~~~~~~
std::error_code fragment_index::end_object () {
    return pop ();
}