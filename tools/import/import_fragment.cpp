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
//===- tools/import/import_fragment.cpp -----------------------------------===//
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
#include "import_fragment.hpp"

#include <unordered_map>
#include <vector>

#include "pstore/support/base64.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/mcrepo/fragment.hpp"
#include "pstore/mcrepo/generic_section.hpp"


#include "array_helpers.hpp"
#include "digest_from_string.hpp"
#include "import_error.hpp"
#include "import_rule.hpp"
#include "import_terminals.hpp"
#include "import_non_terminals.hpp"

using namespace pstore;


//*             _   _                                 *
//*  ___ ___ __| |_(_)___ _ _    _ _  __ _ _ __  ___  *
//* (_-</ -_) _|  _| / _ \ ' \  | ' \/ _` | '  \/ -_) *
//* /__/\___\__|\__|_\___/_||_| |_||_\__,_|_|_|_\___| *
//*                                                   *
class section_name final : public rule {
public:
    section_name (parse_stack_pointer s, gsl::not_null<repo::section_kind *> section)
            : rule (s)
            , section_{section} {}

private:
    gsl::czstring name () const noexcept override;
    std::error_code string_value (std::string const & s) override;

    gsl::not_null<repo::section_kind *> section_;
};

// name
// ~~~~
gsl::czstring section_name::name () const noexcept {
    return "section name";
}

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
class ifixup_rule final : public rule {
public:
    explicit ifixup_rule (parse_stack_pointer s, std::vector<repo::internal_fixup> * fixups)
            : rule (s)
            , fixups_{fixups} {}

private:
    static maybe<repo::section_kind> section_from_string (std::string const & s);

    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    gsl::czstring name () const noexcept override;

    enum { section, type, offset, addend };
    std::bitset<addend + 1> seen_;

    gsl::not_null<std::vector<pstore::repo::internal_fixup> *> fixups_;

    repo::section_kind section_;
    std::uint64_t type_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t addend_ = 0;
};

// name
// ~~~~
gsl::czstring ifixup_rule::name () const noexcept {
    return "ifixup rule";
}

// section from string
// ~~~~~~~~~~~~~~~~~~~
maybe<repo::section_kind> ifixup_rule::section_from_string (std::string const & s) {
#define X(a) {#a, repo::section_kind::a},
    static std::unordered_map<std::string, repo::section_kind> map = {PSTORE_MCREPO_SECTION_KINDS};
#undef X
    auto pos = map.find (s);
    if (pos == map.end ()) {
        return nothing<repo::section_kind> ();
    }
    return just (pos->second);
}

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
    fixups_->emplace_back (section_, static_cast<repo::relocation_type> (type_), offset_, addend_);
    return pop ();
}

//*       __ _                          _      *
//* __ __/ _(_)_ ___  _ _ __   _ _ _  _| |___  *
//* \ \ /  _| \ \ / || | '_ \ | '_| || | / -_) *
//* /_\_\_| |_/_\_\\_,_| .__/ |_|  \_,_|_\___| *
//*                    |_|                     *
class xfixup_rule final : public rule {
public:
    xfixup_rule (parse_stack_pointer s, not_null<std::vector<repo::external_fixup> *> fixups);

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    gsl::czstring name () const noexcept override;

    enum { name_index, type, offset, addend };
    std::bitset<addend + 1> seen_;
    gsl::not_null<std::vector<repo::external_fixup> *> fixups_;

    std::uint64_t name_ = 0;
    std::uint64_t type_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t addend_ = 0;
};

// (ctor)
// ~~~~~~
xfixup_rule::xfixup_rule (parse_stack_pointer s,
                            not_null<std::vector<repo::external_fixup> *> fixups)
        : rule (s)
        , fixups_{fixups} {}

// name
// ~~~~
gsl::czstring xfixup_rule::name () const noexcept {
    return "xfixup rule";
}

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
template <typename OutputIterator>
class generic_section final : public rule {
public:
    generic_section (parse_stack_pointer stack, OutputIterator out)
        : rule(stack), out_{std::move (out)} {}

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    pstore::gsl::czstring name () const noexcept override;

    enum { data, align, ifixups, xfixups };
    std::bitset<xfixups + 1> seen_;
    OutputIterator out_;

    std::string data_;
    std::uint64_t align_ = 0;
    std::vector<pstore::repo::internal_fixup> ifixups_;
    std::vector<pstore::repo::external_fixup> xfixups_;
};

// name
// ~~~~
template <typename OutputIterator>
pstore::gsl::czstring generic_section<OutputIterator>::name () const noexcept {
    return "generic section";
}

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
        return push_array_rule<ifixups_object> (this, &ifixups_);
    }
    if (k == "xfixups") {
        seen_[xfixups] = true;
        return push_array_rule<xfixups_object> (this, &xfixups_);
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

    std::vector<std::uint8_t> data;
    from_base64 (std::begin (data_), std::end (data_), std::back_inserter (data));
    // TODO: fixup offsets must lie inside the data!
//    *out_ = std::make_unique<repo::generic_section_creation_dispatcher> ();
//    content_->align = align_;
//    content_->ifixups = std::move (ifixups_);
//    content_->xfixups = std::move (xfixups_);
    return pop ();
}


//*     _     _                _ _                       _   _           *
//*  __| |___| |__ _  _ __ _  | (_)_ _  ___   ___ ___ __| |_(_)___ _ _   *
//* / _` / -_) '_ \ || / _` | | | | ' \/ -_) (_-</ -_) _|  _| / _ \ ' \  *
//* \__,_\___|_.__/\_,_\__, | |_|_|_||_\___| /__/\___\__|\__|_\___/_||_| *
//*                    |___/                                             *
class debug_line_section final : public rule {
public:
    debug_line_section (parse_stack_pointer stack)
        : rule (stack) {}
private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    gsl::czstring name () const noexcept override;

    enum { header, data, ifixups };
    std::bitset<ifixups + 1> seen_;

    std::string header_;
    std::string data_;
    std::vector<pstore::repo::internal_fixup> ifixups_;
};

// name
// ~~~~
pstore::gsl::czstring debug_line_section::name () const noexcept {
    return "debug line section";
}

// key
// ~~~
std::error_code debug_line_section::key (std::string const & k) {
    if (k == "header") {
        seen_[header] = true;
        return push<string_rule> (&header_);
    }
    if (k == "data") {
        seen_[data] = true; // string (ascii85)
        return push<string_rule> (&data_);
    }
    if (k == "ifixups") {
        seen_[ifixups] = true;
        return push_array_rule<ifixups_object> (this, &ifixups_);
    }
    return import_error::unrecognized_section_object_key;
}

// end object
// ~~~~~~~~~~
std::error_code debug_line_section::end_object () {
    maybe<index::digest> const digest = digest_from_string (header_);
    if (!digest) {
        return import_error::bad_digest;
    }
    if (!seen_.all ()) {
        return import_error::incomplete_debug_line_section;
    }
    return pop ();
}

//*   __                             _                _   _              *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_   ___ ___ __| |_(_)___ _ _  ___ *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| (_-</ -_) _|  _| / _ \ ' \(_-< *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| /__/\___\__|\__|_\___/_||_/__/ *
//*              |___/                                                   *
class fragment_sections final : public rule {
public:
    fragment_sections (parse_stack_pointer s)
            : rule (s) {}
    pstore::gsl::czstring name () const noexcept override;
    std::error_code key (std::string const & s) override;
    std::error_code end_object () override;
private:
    std::vector<std::unique_ptr<repo::section_creation_dispatcher>> sections_;
//    std::unique_ptr<repo::section_content> content_;

};

// name
// ~~~~
pstore::gsl::czstring fragment_sections::name () const noexcept {
    return "fragment sections";
}

// end object
// ~~~~~~~~~~
std::error_code fragment_sections::end_object () {
    return pop ();
}

// key
// ~~~
std::error_code fragment_sections::key (std::string const & s) {
#define X(a) {#a, pstore::repo::section_kind::a},
    static std::unordered_map<std::string, pstore::repo::section_kind> const map{
        PSTORE_MCREPO_SECTION_KINDS
    };
#undef X
    auto const pos = map.find (s);
    if (pos == map.end ()) {
        return import_error::unknown_section_name;
    }
//#define X(a)                                                                                       \
//    case pstore::repo::section_kind::a:                                                            \
//        return create_consumer<                                                                    \
//            pstore::repo::enum_to_section<pstore::repo::section_kind::a>::type>{}(this);

    auto oit = std::back_inserter (sections_);
    switch (pos->second) {
        //PSTORE_MCREPO_SECTION_KINDS
    case repo::section_kind::text:
    case repo::section_kind::data: {
        //content_ = std::make_unique<repo::section_content> (pos->second);
        return push_object_rule<generic_section<decltype(oit)>> (this, oit);
    }
    case repo::section_kind::debug_line:
        //debug_line_ = std::make_unique<repo::debug_line_dispatcher>
    case repo::section_kind::last: assert (false && "Illegal section kind"); // unreachable
    default:
        assert (false && "Unimplemented section kind"); // unreachable
    }
//#undef X
    // digest_ = s; // decode the digest here.
    // return push (std::make_unique<object_consumer<int, next>> (stack (), &x));
    return import_error::unknown_section_name;
}

//*   __                             _     _         _          *
//*  / _|_ _ __ _ __ _ _ __  ___ _ _| |_  (_)_ _  __| |_____ __ *
//* |  _| '_/ _` / _` | '  \/ -_) ' \  _| | | ' \/ _` / -_) \ / *
//* |_| |_| \__,_\__, |_|_|_\___|_||_\__| |_|_||_\__,_\___/_\_\ *
//*              |___/                                          *
// (ctor)
// ~~~~~~
fragment_index::fragment_index (parse_stack_pointer s, transaction_pointer transaction)
        : rule (s)
        , transaction_{transaction} {
    sections_.reserve (static_cast <std::underlying_type_t<repo::section_kind>> (repo::section_kind::last));
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
        return push<object_rule<fragment_sections>> ();
    }
    return import_error::bad_digest;
}

// end object
// ~~~~~~~~~~
std::error_code fragment_index::end_object () {
    return pop ();
}
