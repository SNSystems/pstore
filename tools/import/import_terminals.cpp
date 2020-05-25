#include "import_terminals.hpp"

pstore::gsl::czstring uint64_rule::name () const noexcept {
    return "uint64_rule";
}
pstore::gsl::czstring string_rule::name () const noexcept {
    return "string_rule";
}
