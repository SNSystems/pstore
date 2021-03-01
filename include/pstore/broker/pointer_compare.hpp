//===- include/pstore/broker/pointer_compare.hpp ----------*- mode: C++ -*-===//
//*              _       _                                                        *
//*  _ __   ___ (_)_ __ | |_ ___ _ __    ___ ___  _ __ ___  _ __   __ _ _ __ ___  *
//* | '_ \ / _ \| | '_ \| __/ _ \ '__|  / __/ _ \| '_ ` _ \| '_ \ / _` | '__/ _ \ *
//* | |_) | (_) | | | | | ||  __/ |    | (_| (_) | | | | | | |_) | (_| | | |  __/ *
//* | .__/ \___/|_|_| |_|\__\___|_|     \___\___/|_| |_| |_| .__/ \__,_|_|  \___| *
//* |_|                                                    |_|                    *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
                constexpr explicit helper (Ptr p) noexcept
                        : ptr_ (p) {}
                helper () noexcept = default;
                helper (helper const &) noexcept = default;
                helper (helper &&) noexcept = default;

                template <typename U>
                explicit helper (std::shared_ptr<U> const & sp) noexcept
                        : ptr_{sp.get ()} {}

                template <typename U, typename Deleter = std::default_delete<U>>
                explicit helper (std::unique_ptr<U, Deleter> const & up) noexcept
                        : ptr_{up.get ()} {}

                ~helper () noexcept = default;

                helper & operator= (helper const &) noexcept = default;
                helper & operator= (helper &&) noexcept = default;

                bool operator< (helper other) const { return std::less<Ptr> () (ptr_, other.ptr_); }

            private:
                Ptr ptr_ = nullptr;
            };

            template <typename Lhs, typename Rhs>
            bool operator() (Lhs const & lhs, Rhs const & rhs) const {
                return helper{lhs} < helper{rhs};
            }
        };

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_POINTER_COMPARE_HPP
