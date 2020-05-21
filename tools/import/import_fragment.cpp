#include "import_fragment.hpp"

#include <unordered_map>
#include <vector>

#include "pstore/support/base64.hpp"

#include "array_helpers.hpp"

using namespace pstore;

class ifixup_state final : public state {
public:
    explicit ifixup_state (gsl::not_null<parse_stack *> s,
                           std::vector<repo::internal_fixup> * fixups)
            : state (s)
            , fixups_{fixups} {}

private:
    static maybe<repo::section_kind> section_from_string (std::string const & s);

    std::error_code string_value (std::string const & s) override;
    std::error_code uint64_value (std::uint64_t v) override;
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

    enum { section, type, offset, addend };
    std::bitset<addend + 1> seen_;
    std::uint64_t * v_ = nullptr;
    std::string * str_ = nullptr;

    gsl::not_null<std::vector<pstore::repo::internal_fixup> *> fixups_;

    std::string section_;
    std::uint64_t type_ = 0;
    std::uint64_t offset_ = 0;
    std::uint64_t addend_ = 0;
};

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

std::error_code ifixup_state::string_value (std::string const & s) {
    if (str_ == nullptr) {
        return make_error_code (import_error::unexpected_string);
    }
    *str_ = s;
    str_ = nullptr;
    return {};
}

std::error_code ifixup_state::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        return make_error_code (import_error::unexpected_number);
    }
    *v_ = v;
    v_ = nullptr;
    return {};
}

std::error_code ifixup_state::key (std::string const & k) {
    v_ = nullptr;
    str_ = nullptr;
    if (k == "section") {
        seen_[section] = true;
        str_ = &section_;
        return {};
    }
    if (k == "type") {
        seen_[type] = true;
        v_ = &type_;
        return {};
    }
    if (k == "offset") {
        seen_[offset] = true;
        v_ = &offset_;
        return {};
    }
    if (k == "addend") {
        seen_[addend] = true;
        v_ = &addend_;
        return {};
    }
    return make_error_code (import_error::unrecognized_ifixup_key);
}

std::error_code ifixup_state::end_object () {
    maybe<repo::section_kind> const scn = section_from_string (section_);
    seen_[section] = scn.has_value ();

    if (!seen_.all ()) {
        return make_error_code (import_error::ifixup_object_was_incomplete);
    }
    // TODO: validate more values here.
    fixups_->emplace_back (*scn, static_cast<repo::relocation_type> (type_), offset_, addend_);
    return pop ();
}


class xfixup_state final : public state {
public:
    xfixup_state (gsl::not_null<parse_stack *> s,
                  gsl::not_null<std::vector<repo::external_fixup> *> fixups);

private:
    std::error_code uint64_value (std::uint64_t v) override;
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

    enum { name, type, offset, addend };
    std::bitset<addend + 1> seen_;
    std::uint64_t * v_ = nullptr;
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

std::error_code xfixup_state::key (std::string const & k) {
    if (k == "name") {
        seen_[name] = true;
        v_ = &name_;
    } else if (k == "type") {
        seen_[type] = true;
        v_ = &type_;
    } else if (k == "offset") {
        seen_[offset] = true;
        v_ = &offset_;
    } else if (k == "addend") {
        seen_[addend] = true;
        v_ = &addend_;
    } else {
        return make_error_code (import_error::unrecognized_xfixup_key);
    }
    return {};
}

std::error_code xfixup_state::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        return make_error_code (import_error::unexpected_number);
    }
    *v_ = v;
    v_ = nullptr;
    return {};
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


generic_section::generic_section (gsl::not_null<parse_stack *> stack, gsl::not_null<database *> db)
        : state (stack)
        , db_{db} {}

std::error_code generic_section::string_value (std::string const & s) {
    if (str_ == nullptr) {
        return make_error_code (import_error::unexpected_string);
    }
    *str_ = s;
    str_ = nullptr;
    return {};
}

std::error_code generic_section::uint64_value (std::uint64_t v) {
    if (v_ == nullptr) {
        return make_error_code (import_error::unexpected_number);
    }
    *v_ = v;
    v_ = nullptr;
    return {};
}

std::error_code generic_section::key (std::string const & k) {
    str_ = nullptr;
    v_ = nullptr;

    if (k == "data") {
        seen_[data] = true; // string (ascii85)
        str_ = &data_;
        return {};
    }
    if (k == "align") {
        seen_[align] = true; // integer
        v_ = &align_;
        return {};
    }
    if (k == "ifixups") {
        seen_[ifixups] = true;
        stack ()->push (
            std::make_unique<array_consumer<std::vector<repo::internal_fixup>,
                                            array_object<repo::internal_fixup, ifixup_state>>> (
                stack (), &ifixups_));
        return {};
    }
    if (k == "xfixups") {
        seen_[xfixups] = true;
        stack ()->push (
            std::make_unique<array_consumer<std::vector<repo::external_fixup>,
                                            array_object<repo::external_fixup, xfixup_state>>> (
                stack (), &xfixups_));
        return {};
    }
    return make_error_code (import_error::unrecognized_section_key);
}

std::error_code generic_section::end_object () {
    std::vector<std::uint8_t> data;
    from_base64 (std::begin (data_), std::end (data_), std::back_inserter (data));
    // std::cout << "create generic section\n";
    return {};
}
