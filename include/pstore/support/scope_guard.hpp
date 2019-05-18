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
