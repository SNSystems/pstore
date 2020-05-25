#ifndef PSTORE_IMPORT_IMPORT_TERMINALS_HPP
#define PSTORE_IMPORT_IMPORT_TERMINALS_HPP

#include "import_rule.hpp"

class uint64_rule final : public state {
public:
    uint64_rule (parse_stack_pointer stack, pstore::gsl::not_null<std::uint64_t *> v)
            : state (stack)
            , v_{v} {}
    std::error_code uint64_value (std::uint64_t v) override {
        *v_ = v;
        return pop ();
    }
    pstore::gsl::czstring name () const noexcept override;

private:
    pstore::gsl::not_null<std::uint64_t *> v_;
};

class string_rule final : public state {
public:
    string_rule (parse_stack_pointer stack, pstore::gsl::not_null<std::string *> v)
            : state (stack)
            , v_{v} {}
    std::error_code string_value (std::string const & v) override {
        *v_ = v;
        return pop ();
    }
    pstore::gsl::czstring name () const noexcept override;

private:
    pstore::gsl::not_null<std::string *> v_;
};

#endif // PSTORE_IMPORT_IMPORT_TERMINALS_HPP
