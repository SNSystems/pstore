#include "import_names.hpp"

#include "pstore/core/indirect_string.hpp"

template <typename T>
using not_null = pstore::gsl::not_null<T>;

names::names (transaction_pointer transaction)
        : transaction_{transaction}
        , names_index_{
              pstore::index::get_index<pstore::trailer::indices::name> (transaction->db ())} {}

std::error_code names::add_string (std::string const & str) {
    strings_.push_back (str);
    std::string const & x = strings_.back ();
    views_.emplace_back (pstore::make_sstring_view (x));
    auto & s = views_.back ();
    adder_.add (*transaction_, names_index_, &s);
    return {};
}

void names::flush () {
    adder_.flush (*transaction_);
}


names_array_members::names_array_members (parse_stack_pointer s, not_null<names *> n)
        : state (s)
        , names_{n} {}

std::error_code names_array_members::string_value (std::string const & str) {
    return names_->add_string (str);
}

std::error_code names_array_members::end_array () {
    return pop ();
}

pstore::gsl::czstring names_array_members::name () const noexcept {
    return "names_array_members";
}
