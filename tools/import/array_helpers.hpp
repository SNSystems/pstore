#ifndef PSTORE_IMPORT_ARRAY_HELPERS_HPP
#define PSTORE_IMPORT_ARRAY_HELPERS_HPP

#include "json_callbacks.hpp"

template <typename T, typename Next>
class array_object : public state {
public:
    array_object (pstore::gsl::not_null<parse_stack *> s,
                  pstore::gsl::not_null<std::vector<T> *> arr)
            : state (s)
            , arr_{arr} {}
    std::error_code begin_object () override { return push<Next> (arr_); }
    std::error_code end_array () override { return pop (); }

private:
    pstore::gsl::not_null<std::vector<T> *> arr_;
};

template <typename Output, typename Next>
class array_consumer : public state {
public:
    array_consumer (pstore::gsl::not_null<parse_stack *> s, pstore::gsl::not_null<Output *> out)
            : state (s)
            , out_{out} {}
    std::error_code begin_array () override { return this->replace_top<Next> (out_); }

private:
    pstore::gsl::not_null<Output *> const out_;
};



#endif // PSTORE_IMPORT_ARRAY_HELPERS_HPP
