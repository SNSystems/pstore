//
//  import_non_terminals.hpp
//  gmock
//
//  Created by Paul Bowen-Huggett on 24/05/2020.
//

#ifndef import_non_terminals_hpp
#define import_non_terminals_hpp

#include <functional>
#include "import_rule.hpp"

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
            return details::invoke (std::forward<F> (f), std::get<I> (std::forward<Tuple> (t))...);
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
class object_rule final : public state {
public:
    object_rule (parse_stack_pointer s, Args... args)
            : state (s)
            , args_{std::forward_as_tuple (args...)} {}
    std::error_code begin_object () override {
        cxx17shim::apply (&object_rule::replace_top<NextState, Args...>,
                          std::tuple_cat (std::make_tuple (this), args_));
        return {};
    }

    pstore::gsl::czstring name () const noexcept { return "object_rule"; }

private:
    std::tuple<Args...> args_;
};


template <typename Next, typename... Args>
std::error_code push_object_rule (state * const s, Args &&... args) {
    return s->push<object_rule<Next, Args...>> (std::forward<Args> (args)...);
}


template <typename NextRule, typename... Args>
class array_rule final : public state {
public:
    array_rule (parse_stack_pointer s, Args... args)
            : state (s)
            , args_{std::forward_as_tuple (args...)} {}
    std::error_code begin_array () override {
        cxx17shim::apply (&array_rule::replace_top<NextRule, Args...>,
                          std::tuple_cat (std::make_tuple (this), args_));
        return {};
    }

    pstore::gsl::czstring name () const noexcept { return "array_rule"; }

private:
    std::tuple<Args...> args_;
};

template <typename NextRule, typename... Args>
std::error_code push_array_rule (state * const s, Args &&... args) {
    return s->push<array_rule<NextRule, Args...>> (std::forward<Args> (args)...);
}



#endif /* import_non_terminals_hpp */
