//*  _                            _                        *
//* (_)_ __ ___  _ __   ___  _ __| |_   _ __   ___  _ __   *
//* | | '_ ` _ \| '_ \ / _ \| '__| __| | '_ \ / _ \| '_ \  *
//* | | | | | | | |_) | (_) | |  | |_  | | | | (_) | | | | *
//* |_|_| |_| |_| .__/ \___/|_|   \__| |_| |_|\___/|_| |_| *
//*             |_|                                        *
//*  _                      _             _      *
//* | |_ ___ _ __ _ __ ___ (_)_ __   __ _| |___  *
//* | __/ _ \ '__| '_ ` _ \| | '_ \ / _` | / __| *
//* | ||  __/ |  | | | | | | | | | | (_| | \__ \ *
//*  \__\___|_|  |_| |_| |_|_|_| |_|\__,_|_|___/ *
//*                                              *
//===- tools/import/import_non_terminals.hpp ------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
//===----------------------------------------------------------------------===//
#ifndef PSTORE_IMPORT_IMPORT_NON_TERMINALS_HPP
#define PSTORE_IMPORT_IMPORT_NON_TERMINALS_HPP

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

#endif // PSTORE_IMPORT_IMPORT_NON_TERMINALS_HPP
