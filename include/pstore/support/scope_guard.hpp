//*                                                       _  *
//*  ___  ___ ___  _ __   ___    __ _ _   _  __ _ _ __ __| | *
//* / __|/ __/ _ \| '_ \ / _ \  / _` | | | |/ _` | '__/ _` | *
//* \__ \ (_| (_) | |_) |  __/ | (_| | |_| | (_| | | | (_| | *
//* |___/\___\___/| .__/ \___|  \__, |\__,_|\__,_|_|  \__,_| *
//*               |_|           |___/                        *
//===- include/pstore/support/scope_guard.hpp -----------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_SUPPORT_SCOPE_GUARD_HPP
#define PSTORE_SUPPORT_SCOPE_GUARD_HPP

#include <algorithm>
#include <utility>

#include "pstore/support/portab.hpp"

namespace pstore {

    /// An implementation of std::scope_exit<> which is based loosely on P0052r7 "Generic Scope
    /// Guard and RAII Wrapper for the Standard Library".
    template <typename ExitFunction>
    class scope_guard {
    public:
        template <typename OtherExitFunction>
        explicit scope_guard (OtherExitFunction && other)
                : exit_function_{std::forward<OtherExitFunction> (other)} {}

        scope_guard (scope_guard const &) = delete;
        scope_guard (scope_guard && rhs) noexcept
                : execute_on_destruction_{rhs.execute_on_destruction_}
                , exit_function_{std::move (rhs.exit_function_)} {
            rhs.release ();
        }

        scope_guard & operator= (scope_guard const &) = delete;
        scope_guard & operator= (scope_guard &&) = delete;

        ~scope_guard () noexcept {
            if (execute_on_destruction_) {
                no_ex_escape ([this] () { exit_function_ (); });
            }
        }

        void release () noexcept { execute_on_destruction_ = false; }

    private:
        bool execute_on_destruction_ = true;
        ExitFunction exit_function_;
    };

    template <typename Function>
    scope_guard<typename std::decay<Function>::type> make_scope_guard (Function && f) {
        return scope_guard<typename std::decay<Function>::type> (std::forward<Function> (f));
    }

} // namespace pstore

#endif // PSTORE_SUPPORT_SCOPE_GUARD_HPP
