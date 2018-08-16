//*                                             *
//*   ___ ___  _ __ ___  _ __ ___   ___  _ __   *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \| '_ \  *
//* | (_| (_) | | | | | | | | | | | (_) | | | | *
//*  \___\___/|_| |_| |_|_| |_| |_|\___/|_| |_| *
//*                                             *
//===- include/pstore/serialize/common.hpp --------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
            sticky_assign ()
                    : t_{} {}
            /// Construct from a sticky_assign<T>. This is considered equivalent to assignment.
            sticky_assign (sticky_assign<T> const & rhs)
                    : t_ (rhs.get ())
                    , is_first_{false} {}

            /// Construct from a type that is implicitly convertible to T. This is considered
            /// equivalent to assignment.
            template <typename Other>
            sticky_assign (Other const & t)
                    : t_ (t)
                    , is_first_{false} {}

            /// Assign from a type that is implicitly convertable to T. This
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

            T const & get () const { return t_; }

        private:
            T t_;
            bool is_first_ = true;
        };

    } // namespace serialize
} // namespace pstore
#endif // PSTORE_SERIALIZE_COMMON_HPP
