#ifndef PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
#define PSTORE_IMPORT_IMPORT_FRAGMENT_HPP

#include <bitset>
#include <vector>

#include "pstore/core/database.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "json_callbacks.hpp"

class generic_section final : public state {
public:
    generic_section (pstore::gsl::not_null<parse_stack *> stack);

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    pstore::gsl::czstring name () const noexcept override;

    enum { data, align, ifixups, xfixups };
    std::bitset<xfixups + 1> seen_;

    std::string data_;
    std::uint64_t align_ = 0;
    std::vector<pstore::repo::internal_fixup> ifixups_;
    std::vector<pstore::repo::external_fixup> xfixups_;
};

class debug_line_section final : public state {
public:
    debug_line_section (pstore::gsl::not_null<parse_stack *> stack);

private:
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;
    pstore::gsl::czstring name () const noexcept override;

    enum { header, data, ifixups };
    std::bitset<ifixups + 1> seen_;

    std::string header_;
    std::string data_;
    std::vector<pstore::repo::internal_fixup> ifixups_;
};

#endif // PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
