#ifndef json_callbacks_hpp
#define json_callbacks_hpp

#include <stack>
#include <system_error>
#include <tuple>

#include "pstore/support/gsl.hpp"


enum class import_error : int {
    none,

    unexpected_null,
    unexpected_boolean,
    unexpected_number,
    unexpected_string,
    unexpected_array,
    unexpected_end_array,
    unexpected_object,
    unexpected_object_key,
    unexpected_end_object,

    unrecognized_ifixup_key,
    ifixup_object_was_incomplete,
    unrecognized_xfixup_key,
    xfixup_object_was_incomplete,
    unrecognized_section_key,
    unrecognized_root_key,
    root_object_was_incomplete,
    unknown_transaction_object_key,
};


class import_error_category : public std::error_category {
public:
    import_error_category () noexcept;
    char const * name () const noexcept override;
    std::string message (int error) const override;
};

std::error_category const & get_import_error_category () noexcept;
std::error_code make_error_code (import_error const e) noexcept;


namespace std {

    template <>
    struct is_error_code_enum<import_error> : std::true_type {};

} // end namespace std


namespace details {

    // TODO: remove when we switch to C++17.
    template <typename Tuple, typename F, std::size_t... Is>
    constexpr auto apply_impl (F f, Tuple t, std::index_sequence<Is...>) {
        return f (std::get<Is> (t)...);
    }

    // TODO: remove when we switch to C++17.
    template <typename F, typename Tuple>
    constexpr auto apply (F f, Tuple t) {
        return apply_impl (f, t, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

} // end namespace details

template <typename Next, typename... Args>
class object_consumer;

template <typename Next, typename... Args>
auto make_object_consumer (Args &&... args);



class state;
using parse_stack = std::stack<std::unique_ptr<state>>;

class state {
public:
    explicit state (pstore::gsl::not_null<parse_stack *> s)
            : stack_{s} {}
    virtual ~state ();

    virtual std::error_code int64_value (std::int64_t v);
    virtual std::error_code uint64_value (std::uint64_t v);
    virtual std::error_code double_value (double v);
    virtual std::error_code string_value (std::string const & v);
    virtual std::error_code boolean_value (bool v);
    virtual std::error_code null_value ();
    virtual std::error_code begin_array ();
    virtual std::error_code end_array ();
    virtual std::error_code begin_object ();
    virtual std::error_code key (std::string const & k);
    virtual std::error_code end_object ();

protected:
    pstore::gsl::not_null<parse_stack *> stack () { return stack_; }

    template <typename T, typename... Args>
    std::error_code push (Args &&... args) {
        stack_->push (std::make_unique<T> (stack (), std::forward<Args> (args)...));
    }


    template <typename T, typename... Args>
    std::error_code replace_top (Args &&... args) {
        auto stack = stack_;
        auto p = std::make_unique<T> (stack, std::forward<Args> (args)...);
        stack->pop (); // destroys this object!
        stack->push (std::move (p));
        return {};
    }

    std::error_code pop () {
        stack_->pop ();
        return {};
    }

private:
    pstore::gsl::not_null<parse_stack *> stack_;
};


template <typename Next, typename... Args>
class object_consumer : public state {
public:
    object_consumer (pstore::gsl::not_null<parse_stack *> s, Args... args)
            : state (s)
            , args_{std::forward_as_tuple (args...)} {}
    std::error_code begin_object () override {
        // auto fn = [this] (args...) { this->replace_top<Next> (args...); }
        // auto x = &this->replace_top<Next>;
        std::apply (&object_consumer::replace_top<Next>,
                    std::tuple_cat (std::make_tuple (this), args_));
        return {};
    }

private:
    std::tuple<Args...> args_;
};


template <typename Next, typename... Args>
auto make_object_consumer (Args &&... args) {
    return object_consumer<Next, Args...> (std::forward<Args> (args)...);
}


class expect_uint64 final : public state {
public:
    expect_uint64 (pstore::gsl::not_null<parse_stack *> stack,
                   pstore::gsl::not_null<std::uint64_t *> v)
            : state (stack)
            , v_{v} {}
    std::error_code uint64_value (std::uint64_t v) override {
        *v_ = v;
        return pop ();
    }
private:
    pstore::gsl::not_null<std::uint64_t *> v_;
};

class expect_string final : public state {
public:
    expect_string (pstore::gsl::not_null<parse_stack *> stack,
                   pstore::gsl::not_null<std::string *> v)
            : state (stack)
            , v_{v} {}
    std::error_code string_value (std::string const & v) override {
        *v_ = v;
        return pop ();
    }

private:
    pstore::gsl::not_null<std::string *> v_;
};


class callbacks {
public:
    using result_type = void;
    result_type result () {}

    template <typename Root, typename... Args>
    static callbacks make (Args &&... args) {
        auto stack = std::make_shared<parse_stack> ();
        return {stack, std::make_unique<Root> (stack.get (), std::forward<Args> (args)...)};
    }

    std::error_code int64_value (std::int64_t v) {
        assert (!stack_->empty ());
        return stack_->top ()->int64_value (v);
    }
    std::error_code uint64_value (std::uint64_t v) {
        assert (!stack_->empty ());
        return stack_->top ()->uint64_value (v);
    }
    std::error_code double_value (double v) { return stack_->top ()->double_value (v); }
    std::error_code string_value (std::string const & v) {
        return stack_->top ()->string_value (v);
    }
    std::error_code boolean_value (bool v) { return stack_->top ()->boolean_value (v); }
    std::error_code null_value () { return stack_->top ()->null_value (); }
    std::error_code begin_array () { return stack_->top ()->begin_array (); }
    std::error_code end_array () { return stack_->top ()->end_array (); }
    std::error_code begin_object () { return stack_->top ()->begin_object (); }
    std::error_code key (std::string const & k) { return stack_->top ()->key (k); }
    std::error_code end_object () { return stack_->top ()->end_object (); }

private:
    callbacks (std::shared_ptr<parse_stack> const & stack, std::unique_ptr<state> && root)
            : stack_{stack} {
        stack_->push (std::move (root));
    }

    std::shared_ptr<parse_stack> stack_;
};

#endif /* json_callbacks_hpp */
