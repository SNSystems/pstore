#ifndef PSTORE_IMPORT_IMPORT_NAMES_HPP
#define PSTORE_IMPORT_IMPORT_NAMES_HPP

#include <list>
#include <system_error>

#include "pstore/core/hamt_set.hpp"
#include "pstore/core/index_types.hpp"
#include "pstore/core/transaction.hpp"

#include "import_rule.hpp"

using transaction_type = pstore::transaction<pstore::transaction_lock>;
using transaction_pointer = pstore::gsl::not_null<transaction_type *>;

class names {
public:
    explicit names (transaction_pointer transaction);

    std::error_code add_string (std::string const & str);
    void flush ();

private:
    transaction_pointer transaction_;
    std::shared_ptr<pstore::index::name_index> names_index_;

    pstore::indirect_string_adder adder_;
    std::list<std::string> strings_;
    std::list<pstore::raw_sstring_view> views_;
};

using names_pointer = pstore::gsl::not_null<names *>;


class names_array_members final : public state {
public:
    names_array_members (parse_stack_pointer s, names_pointer n);

private:
    std::error_code string_value (std::string const & str) override;
    std::error_code end_array () override;
    pstore::gsl::czstring name () const noexcept override;

    names_pointer names_;
};

#endif // PSTORE_IMPORT_IMPORT_NAMES_HPP
