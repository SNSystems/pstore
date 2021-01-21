//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- include/pstore/serialize/common.hpp --------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_SERIALIZE_COMMON_HPP
#define PSTORE_SERIALIZE_COMMON_HPP

namespace pstore {
    namespace serialize {

        /// \brief A helper class which remembers the first time that it is assigned to.
        /// This class is intended to be used to simplify loops of the general form:
        ///
        ///     result_type result;
        ///     bool is_first = true;
        ///     for (auto & v : range) {
        ///         auto r = ... produce a value ...
        ///         if (is_first) {
        ///             result = r;
        ///         }
        ///         is_first = false;
        ///     }
        ///     return result;
        ///
        /// This rather error prone snippet of code can be replaced with:
        ///
        ///     sticky_type <result_type> r;
        ///     for (auto & v : range) {
        ///         r = ... produce a value ...
        ///     }
        ///     return r.get ();

        template <typename T>
        class sticky_assign {
        public:
            sticky_assign () = default;
            /// Construct from a sticky_assign<T>. This is considered equivalent to assignment.
            sticky_assign (sticky_assign<T> const & rhs)
                    : t_ (rhs.get ())
                    , is_first_{false} {}

            /// Construct from a type that is implicitly convertible to T. This is considered
            /// equivalent to assignment.
            template <typename Other>
            explicit sticky_assign (Other const & t)
                    : t_ (t)
                    , is_first_{false} {}
            sticky_assign (sticky_assign &&) = delete;

            ~sticky_assign () = default;

            /// Assign from a type that is implicitly convertible to T. This
            /// assignment will take place once only: any subsequent assignments
            /// will be ignored.
            template <typename Other>
            sticky_assign & operator= (Other const & rhs) {
                if (is_first_) {
                    t_ = rhs;
                    is_first_ = false;
                }
                return *this;
            }
            sticky_assign & operator= (sticky_assign const &) = delete;
            sticky_assign & operator= (sticky_assign &&) = delete;

            T const & get () const { return t_; }

        private:
            T t_{};
            bool is_first_ = true;
        };

    } // namespace serialize
} // namespace pstore
#endif // PSTORE_SERIALIZE_COMMON_HPP
