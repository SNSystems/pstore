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
//===- include/pstore/exchange/import_non_terminals.hpp -------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP
#define PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP

#include "pstore/exchange/import_rule.hpp"

namespace pstore {
    namespace exchange {

        namespace cxx17shim {

#if __cplusplus < 201703L
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
                constexpr decltype (auto) apply_impl (F && f, Tuple && t,
                                                      std::index_sequence<I...>) {
                    return details::invoke (std::forward<F> (f),
                                            std::get<I> (std::forward<Tuple> (t))...);
                }

            } // end namespace details

            template <typename F, typename Tuple>
            constexpr decltype (auto) apply (F && f, Tuple && t) {
                using indices =
                    std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
                return details::apply_impl (std::forward<F> (f), std::forward<Tuple> (t),
                                            indices{});
            }

#else
            template <typename F, typename Tuple>
            using apply = std::apply<F, Tuple>;
#endif

        } // end namespace cxx17shim

        namespace import {

            //*      _     _        _              _      *
            //*  ___| |__ (_)___ __| |_   _ _ _  _| |___  *
            //* / _ \ '_ \| / -_) _|  _| | '_| || | / -_) *
            //* \___/_.__// \___\__|\__| |_|  \_,_|_\___| *
            //*         |__/                              *
            //-MARK: object rule
            template <typename NextState, typename... Args>
            class object_rule final : public rule {
            public:
                explicit object_rule (not_null<context *> const ctxt, Args... args)
                        : rule (ctxt)
                        , args_{std::forward_as_tuple (args...)} {}
                object_rule (object_rule const &) = delete;
                object_rule (object_rule &&) noexcept = delete;

                ~object_rule () noexcept override = default;

                object_rule & operator= (object_rule const &) = delete;
                object_rule & operator= (object_rule &&) noexcept = delete;

                gsl::czstring name () const noexcept override { return "object rule"; }

                std::error_code begin_object () override {
                    cxx17shim::apply (&object_rule::replace_top<NextState, Args...>,
                                      std::tuple_cat (std::make_tuple (this), args_));
                    return {};
                }

            private:
                std::tuple<Args...> args_;
            };

            template <typename Next, typename... Args>
            std::error_code push_object_rule (rule * const rule, Args... args) {
                return rule->push<object_rule<Next, Args...>> (args...);
            }


            //*                                    _      *
            //*  __ _ _ _ _ _ __ _ _  _   _ _ _  _| |___  *
            //* / _` | '_| '_/ _` | || | | '_| || | / -_) *
            //* \__,_|_| |_| \__,_|\_, | |_|  \_,_|_\___| *
            //*                    |__/                   *
            //-MARK: array rule
            template <typename NextRule, typename... Args>
            class array_rule final : public rule {
            public:
                explicit array_rule (not_null<context *> const ctxt, Args... args)
                        : rule (ctxt)
                        , args_{std::forward_as_tuple (args...)} {}
                array_rule (array_rule const &) = delete;
                array_rule (array_rule &&) noexcept = delete;

                ~array_rule () noexcept override = default;

                array_rule & operator= (array_rule const &) = delete;
                array_rule & operator= (array_rule &&) noexcept = delete;

                std::error_code begin_array () override {
                    cxx17shim::apply (&array_rule::replace_top<NextRule, Args...>,
                                      std::tuple_cat (std::make_tuple (this), args_));
                    return {};
                }

                gsl::czstring name () const noexcept override { return "array rule"; }

            private:
                std::tuple<Args...> args_;
            };

            template <typename NextRule, typename... Args>
            std::error_code push_array_rule (rule * const rule, Args... args) {
                return rule->push<array_rule<NextRule, Args...>> (args...);
            }

        } // end namespace import
    }     // end namespace exchange
} // end namespace pstore

#endif // PSTORE_EXCHANGE_IMPORT_NON_TERMINALS_HPP
