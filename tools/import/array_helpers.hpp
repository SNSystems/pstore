#ifndef array_helpers_h
#define array_helpers_h

#include "json_callbacks.hpp"

template <typename T, typename Next>
class array_object : public state {
public:
    array_object (pstore::gsl::not_null<parse_stack *> s,
                  pstore::gsl::not_null<std::vector<T> *> arr)
            : state (s)
            , arr_{arr} {}
    std::error_code begin_object () override {
        stack ()->push (std::make_unique<Next> (stack (), arr_));
        return {};
    }
    std::error_code end_array () override { return this->suicide (); }

private:
    pstore::gsl::not_null<std::vector<T> *> arr_;
};

template <typename Output, typename Next>
class array_consumer : public state {
public:
    array_consumer (pstore::gsl::not_null<parse_stack *> s, pstore::gsl::not_null<Output *> out)
            : state (s)
            , out_{out} {}
    std::error_code begin_array () override {
        auto stack = this->stack ();
        auto out = out_;
        stack->pop (); // suicide
        stack->push (std::make_unique<Next> (stack, out));
        return {};
    }

private:
    pstore::gsl::not_null<Output *> const out_;
};

#endif /* array_helpers_h */
