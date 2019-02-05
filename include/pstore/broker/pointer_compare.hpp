//*              _       _                                                        *
//*  _ __   ___ (_)_ __ | |_ ___ _ __    ___ ___  _ __ ___  _ __   __ _ _ __ ___  *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__|  / __/ _ \| '_ ` _ \| '_ \ / _` | '__/ _ \ *
//* | |_) | (_) | | | | | ||  __/ |    | (_| (_) | | | | | | |_) | (_| | | |  __/ *
//* | .__/ \___/|_|_| |_|\__\___|_|     \___\___/|_| |_| |_| .__/ \__,_|_|  \___| *
//* |_|                                                    |_|                    *
//===- include/pstore/broker/pointer_compare.hpp --------------------------===//
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
/// \file pointer_compare.hpp

#ifndef PSTORE_BROKER_POINTER_COMPARE_HPP
#define PSTORE_BROKER_POINTER_COMPARE_HPP

#include <functional>
#include <memory>
#include <type_traits>

namespace pstore {
    namespace broker {

        /// A type which can used by the ordered containers (set<>, map<>, and the local bimap<>) to
        /// perform key comparison where the keys are one of the various types of smart or raw
        /// pointer.
        /// shared_ptr<> and unique_ptr<> instances are converted to the underlying raw type before
        /// comparison.
        template <typename Ptr>
        class pointer_compare {
        public:
            static_assert (std::is_pointer<Ptr>::value, "Ptr must be a pointer type");
            using is_transparent = std::true_type;

            // This helper class turns the various smart and raw pointers into raw pointers and then
            // uses std::less<Ptr> to compare them.
            class helper {
            public:
                helper ()
                        : ptr_ (nullptr) {}
                helper (helper const &) = default;
                helper (Ptr p)
                        : ptr_ (p) {}

                template <typename U>
                helper (std::shared_ptr<U> const & sp)
                        : ptr_{sp.get ()} {}

                template <typename U, typename Deleter = std::default_delete<U>>
                helper (std::unique_ptr<U, Deleter> const & up)
                        : ptr_{up.get ()} {}

                bool operator< (helper other) const { return std::less<Ptr> () (ptr_, other.ptr_); }

            private:
                Ptr ptr_;
            };

            bool operator() (helper const & lhs, helper const & rhs) const { return lhs < rhs; }
        };

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_POINTER_COMPARE_HPP
