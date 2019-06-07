//*                                                       _  *
//*  ___  ___ ___  _ __   ___    __ _ _   _  __ _ _ __ __| | *
//* / __|/ __/ _ \| '_ \ / _ \  / _` | | | |/ _` | '__/ _` | *
//* \__ \ (_| (_) | |_) |  __/ | (_| | |_| | (_| | | | (_| | *
//* |___/\___\___/| .__/ \___|  \__, |\__,_|\__,_|_|  \__,_| *
//*               |_|           |___/                        *
//===- include/pstore/support/scope_guard.hpp -----------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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

        scope_guard (scope_guard && rhs) noexcept
                : execute_on_destruction_{rhs.execute_on_destruction_}
                , exit_function_{std::move (rhs.exit_function_)} {
            rhs.release ();
        }

        ~scope_guard () noexcept {
            if (execute_on_destruction_) {
                PSTORE_NO_EX_ESCAPE (exit_function_ ());
            }
        }

        void release () noexcept { execute_on_destruction_ = false; }

        scope_guard (scope_guard const &) = delete;
        scope_guard & operator= (scope_guard const &) = delete;
        scope_guard & operator= (scope_guard &&) = delete;

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
