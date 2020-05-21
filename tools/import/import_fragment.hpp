#ifndef PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
#define PSTORE_IMPORT_IMPORT_FRAGMENT_HPP

#include <bitset>
#include <vector>

#include "pstore/core/database.hpp"
#include "pstore/mcrepo/generic_section.hpp"
#include "json_callbacks.hpp"

class generic_section final : public state {
public:
    generic_section (pstore::gsl::not_null<parse_stack *> stack,
                     pstore::gsl::not_null<pstore::database *> db);

    std::error_code string_value (std::string const & s);
    std::error_code uint64_value (std::uint64_t v) override;
    std::error_code key (std::string const & k) override;
    std::error_code end_object () override;

private:
    enum { data, align, ifixups, xfixups };
    std::bitset<xfixups + 1> seen_;
    std::uint64_t * v_ = nullptr;
    std::string * str_ = nullptr;

    pstore::gsl::not_null<pstore::database const *> db_;
    std::string data_;
    std::uint64_t align_ = 0;
    std::vector<pstore::repo::internal_fixup> ifixups_;
    std::vector<pstore::repo::external_fixup> xfixups_;
};

#endif // PSTORE_IMPORT_IMPORT_FRAGMENT_HPP
