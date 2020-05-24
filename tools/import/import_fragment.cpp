#include "import_fragment.hpp"

#include <unordered_map>
#include <vector>

#include "pstore/support/base64.hpp"
#include "pstore/core/index_types.hpp"

#include "array_helpers.hpp"
#include "digest_from_string.hpp"

using namespace pstore;

class section_name final : public state {
public:
    section_name (gsl::not_null<parse_stack *> s, gsl::not_null<repo::section_kind *> section)
            : state (s)
            , section_{section} {}

private:
    std::error_code string_value (std::string const & s) override {
        // TODO: this map appears both here and in the fragment code.
#define X(a) {#a, repo::section_kind::a},
        static std::unordered_map<std::string, repo::section_kind> map = {
            PSTORE_MCREPO_SECTION_KINDS};
#undef X
        auto pos = map.find (s);
        if (pos == map.end ()) {
            return make_error_code (import_error::unknown_section_name);
        }
        *section_ = pos->second;
        return pop ();
    }
    gsl::czstring name () const noexcept override { return "section_name"; }

    gsl::not_null<repo::section_kind *> section_;
};

class ifixup_state final : public state {
public:
    explicit ifixup_state (gsl::not_null<parse_stack *> s,
                           std::vector<repo::internal_fixup> * fixups)
            : state (s)
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

gsl::czstring ifixup_state::name () const noexcept {
    return "ifixup_state";
}

maybe<repo::section_kind> ifixup_state::section_from_string (std::string const & s) {
#define X(a) {#a, repo::section_kind::a},
    static std::unordered_map<std::string, repo::section_kind> map = {PSTORE_MCREPO_SECTION_KINDS};
#undef X
    auto pos = map.find (s);
    if (pos == map.end ()) {
        return nothing<repo::section_kind> ();
    }
    return just (pos->second);
}

std::error_code ifixup_state::key (std::string const & k) {
    if (k == "section") {
        seen_[section] = true;
        return push<section_name> (&section_);
    }
    if (k == "type") {
        seen_[type] = true;
        return push<expect_uint64> (&type_);
    }
    if (k == "offset") {
        seen_[offset] = true;
        return push<expect_uint64> (&offset_);
    }
    if (k == "addend") {
        seen_[addend] = true;
        return push<expect_uint64> (&addend_);
    }
    return make_error_code (import_error::unrecognized_ifixup_key);
}

std::error_code ifixup_state::end_object () {
    if (!seen_.all ()) {
        return make_error_code (import_error::ifixup_object_was_incomplete);
    }
    // TODO: validate more values here.
    fixups_->emplace_back (section_, static_cast<repo::relocation_type> (type_), offset_, addend_);
    return pop ();
}


class xfixup_state final : public state {
public:
    xfixup_state (gsl::not_null<parse_stack *> s,
                  gsl::not_null<std::vector<repo::external_fixup> *> fixups);
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

xfixup_state::xfixup_state (gsl::not_null<parse_stack *> s,
                            gsl::not_null<std::vector<repo::external_fixup> *> fixups)
        : state (s)
        , fixups_{fixups} {}

gsl::czstring xfixup_state::name () const noexcept {
    return "xfixup_state";
}

std::error_code xfixup_state::key (std::string const & k) {
    if (k == "name") {
        seen_[name_index] = true;
        return push<expect_uint64> (&name_);
    }
    if (k == "type") {
        seen_[type] = true;
        return push<expect_uint64> (&type_);
    }
    if (k == "offset") {
        seen_[offset] = true;
        return push<expect_uint64> (&offset_);
    }
    if (k == "addend") {
        seen_[addend] = true;
        return push<expect_uint64> (&addend_);
    }
    return make_error_code (import_error::unrecognized_xfixup_key);
}

std::error_code xfixup_state::end_object () {
    if (!seen_.all ()) {
        return make_error_code (import_error::xfixup_object_was_incomplete);
    }
    // TODO: validate some values here.
    fixups_->emplace_back (typed_address<indirect_string>::make (name_),
                           static_cast<repo::relocation_type> (type_), offset_, addend_);
    return pop ();
}

template <typename Next, typename Fixup>
class fixups_object final : public state {
public:
    fixups_object (gsl::not_null<parse_stack *> stack, gsl::not_null<std::vector<Fixup> *> fixups)
            : state (stack)
            , fixups_{fixups} {}
    gsl::czstring name () const noexcept override { return "fixups_object"; }
    std::error_code begin_object () override { return push<Next> (fixups_); }
    std::error_code end_array () override { return pop (); }

private:
    gsl::not_null<std::vector<Fixup> *> fixups_;
};

using ifixups_object = fixups_object<ifixup_state, repo::internal_fixup>;
using xfixups_object = fixups_object<xfixup_state, repo::external_fixup>;

generic_section::generic_section (gsl::not_null<parse_stack *> stack)
        : state (stack) {}

pstore::gsl::czstring generic_section::name () const noexcept {
    return "generic_section";
}

std::error_code generic_section::key (std::string const & k) {
    if (k == "data") {
        seen_[data] = true; // string (ascii85)
        return push<expect_string> (&data_);
    }
    if (k == "align") {
        seen_[align] = true; // integer
        return this->push<expect_uint64> (&align_);
    }
    if (k == "ifixups") {
        seen_[ifixups] = true;
        return push_array_consumer<ifixups_object> (this, &ifixups_);
    }
    if (k == "xfixups") {
        seen_[xfixups] = true;
        return push_array_consumer<xfixups_object> (this, &xfixups_);
    }
    return make_error_code (import_error::unrecognized_section_object_key);
}

std::error_code generic_section::end_object () {
    if (!seen_.all ()) {
        return make_error_code (import_error::generic_section_was_incomplete);
    }
    std::vector<std::uint8_t> data;
    from_base64 (std::begin (data_), std::end (data_), std::back_inserter (data));
    // std::cout << "create generic section\n";

    // TODO: align must be a power of 2!
    // TODO: offsets must lie inside the data!
    return pop ();
}



debug_line_section::debug_line_section (pstore::gsl::not_null<parse_stack *> stack)
        : state (stack) {}

pstore::gsl::czstring debug_line_section::name () const noexcept {
    return "debug_line_section";
}

std::error_code debug_line_section::key (std::string const & k) {
    if (k == "header") {
        seen_[header] = true;
        return push<expect_string> (&header_);
    }
    if (k == "data") {
        seen_[data] = true; // string (ascii85)
        return push<expect_string> (&data_);
    }
    if (k == "ifixups") {
        seen_[ifixups] = true;
        return push_array_consumer<ifixups_object> (this, &ifixups_);
    }
    return import_error::unrecognized_section_object_key;
}
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
