#ifndef json_callbacks_hpp
#define json_callbacks_hpp

#include <iostream>
#include <stack>
#include <system_error>
#include <tuple>

#include "pstore/support/gsl.hpp"

template <typename T>
using not_null = pstore::gsl::not_null<T>;

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
    generic_section_was_incomplete,
    unrecognized_section_object_key,
    unrecognized_root_key,
    root_object_was_incomplete,
    unknown_transaction_object_key,
    unknown_compilation_object_key,
    unknown_definition_object_key,

    incomplete_compilation_object,
    incomplete_debug_line_section,

    bad_digest,
    bad_base64_data,
    bad_linkage,
    bad_visibility,
    unknown_section_name,
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


class state {
public:
    using parse_stack = std::stack<std::unique_ptr<state>>;

    explicit state (not_null<parse_stack *> stack)
            : stack_{stack} {}
    state (state const &) = delete;
    virtual ~state ();

    virtual pstore::gsl::czstring name () const noexcept = 0;

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

    // protected:
    template <typename T, typename... Args>
    std::error_code push (Args &&... args) {
        stack_->push (std::make_unique<T> (stack_, std::forward<Args> (args)...));
        std::cout << std::string (stack_->size () * trace_indent, ' ') << '+'
                  << stack_->top ()->name () << '\n';
        return {};
    }

    template <typename T, typename... Args>
    std::error_code replace_top (Args &&... args) {
        auto p = std::make_unique<T> (stack_, std::forward<Args> (args)...);
        auto stack = stack_;
        std::cout << indent (*stack_) << '-' << stack_->top ()->name () << '\n';

        stack->pop (); // Destroys this object.
        stack->push (std::move (p));
        std::cout << indent (*stack) << '+' << stack->top ()->name () << '\n';
        return {};
    }

    std::error_code pop () {
        std::cout << indent (*stack_) << '-' << stack_->top ()->name () << '\n';
        stack_->pop ();
        return {};
    }

private:
    static constexpr auto trace_indent = std::size_t{2};
    static std::string indent (parse_stack const & stack) {
        return std::string (stack.size () * trace_indent, ' ');
    }
    not_null<parse_stack *> stack_;
};


#if __cplusplus < 201703L
namespace cxx17shim {

    namespace details {

        template <typename Fn, typename... Args,
                  std::enable_if_t<std::is_member_pointer<std::decay_t<Fn>>{}, int> = 0>
        constexpr decltype (auto) invoke (Fn && f, Args &&... args) noexcept (
            noexcept (std::mem_fn (f) (std::forward<Args> (args)...))) {
            return std::mem_fn (f) (std::forward<Args> (args)...);
        }

        template <typename Fn, typename... Args,
                  std::enable_if_t<!std::is_member_pointer<std::decay_t<Fn>>{}, int> = 0>
        constexpr decltype (auto) invoke (Fn && f, Args &&... args) noexcept (
            noexcept (std::forward<Fn> (f) (std::forward<Args> (args)...))) {
            return std::forward<Fn> (f) (std::forward<Args> (args)...);
        }

        template <typename F, typename Tuple, size_t... I>
        constexpr decltype (auto) apply_impl (F && f, Tuple && t, std::index_sequence<I...>) {
            return invoke (std::forward<F> (f), std::get<I> (std::forward<Tuple> (t))...);
        }

    } // end namespace details

    template <typename F, typename Tuple>
    constexpr decltype (auto) apply (F && f, Tuple && t) {
        using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
        return details::apply_impl (std::forward<F> (f), std::forward<Tuple> (t), Indices{});
    }

} // end namespace cxx17shim


#else

namespace cxx17shim {

    template <typename F, typename Tuple>
    using apply = std::apply<F, Tuple>;

} // end namespace cxx17shim

#endif

template <typename NextState, typename... Args>
class object_consumer final : public state {
public:
    object_consumer (not_null<parse_stack *> s, Args... args)
            : state (s)
            , args_{std::forward_as_tuple (args...)} {}
    std::error_code begin_object () override {
        cxx17shim::apply (&object_consumer::replace_top<NextState, Args...>,
                          std::tuple_cat (std::make_tuple (this), args_));
        return {};
    }

    pstore::gsl::czstring name () const noexcept { return "object_consumer"; }

private:
    std::tuple<Args...> args_;
};


template <typename Next, typename... Args>
std::error_code push_object_consumer (state * const s, Args &&... args) {
    return s->push<object_consumer<Next, Args...>> (std::forward<Args> (args)...);
}


template <typename NextState, typename... Args>
class array_consumer final : public state {
public:
    array_consumer (not_null<parse_stack *> s, Args... args)
            : state (s)
            , args_{std::forward_as_tuple (args...)} {}
    std::error_code begin_array () override {
        cxx17shim::apply (&array_consumer::replace_top<NextState, Args...>,
                          std::tuple_cat (std::make_tuple (this), args_));
        return {};
    }

    pstore::gsl::czstring name () const noexcept { return "array_consumer"; }

private:
    std::tuple<Args...> args_;
};

template <typename Next, typename... Args>
std::error_code push_array_consumer (state * const s, Args &&... args) {
    return s->push<array_consumer<Next, Args...>> (std::forward<Args> (args)...);
}



class expect_uint64 final : public state {
public:
    expect_uint64 (not_null<parse_stack *> stack, not_null<std::uint64_t *> v)
            : state (stack)
            , v_{v} {}
    std::error_code uint64_value (std::uint64_t v) override {
        *v_ = v;
        return pop ();
    }
    pstore::gsl::czstring name () const noexcept { return "expect_uint64"; }

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
    pstore::gsl::czstring name () const noexcept { return "expect_string"; }

private:
    pstore::gsl::not_null<std::string *> v_;
};


class callbacks {
public:
    using result_type = void;
    result_type result () {}

    template <typename Root, typename... Args>
    static callbacks make (Args &&... args) {
        auto stack = std::make_shared<state::parse_stack> ();
        return {stack, std::make_unique<Root> (stack.get (), std::forward<Args> (args)...)};
    }

    std::error_code int64_value (std::int64_t v) { return top ()->int64_value (v); }
    std::error_code uint64_value (std::uint64_t v) { return top ()->uint64_value (v); }
    std::error_code double_value (double v) { return top ()->double_value (v); }
    std::error_code string_value (std::string const & v) { return top ()->string_value (v); }
    std::error_code boolean_value (bool v) { return top ()->boolean_value (v); }
    std::error_code null_value () { return top ()->null_value (); }
    std::error_code begin_array () { return top ()->begin_array (); }
    std::error_code end_array () { return top ()->end_array (); }
    std::error_code begin_object () { return top ()->begin_object (); }
    std::error_code key (std::string const & k) { return top ()->key (k); }
    std::error_code end_object () { return top ()->end_object (); }

private:
    callbacks (std::shared_ptr<state::parse_stack> const & stack, std::unique_ptr<state> && root)
            : stack_{stack} {
        stack_->push (std::move (root));
    }

    std::unique_ptr<state> & top () {
        assert (!stack_->empty ());
        return stack_->top ();
    }
    std::shared_ptr<state::parse_stack> stack_;
};

#endif /* json_callbacks_hpp */
