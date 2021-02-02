//*  _                     _                              __             _  *
//* | |__   __ _ _ __ ___ | |_   _ __ ___   __ _ _ __    / _|_      ____| | *
//* | '_ \ / _` | '_ ` _ \| __| | '_ ` _ \ / _` | '_ \  | |_\ \ /\ / / _` | *
//* | | | | (_| | | | | | | |_  | | | | | | (_| | |_) | |  _|\ V  V / (_| | *
//* |_| |_|\__,_|_| |_| |_|\__| |_| |_| |_|\__,_| .__/  |_|   \_/\_/ \__,_| *
//*                                             |_|                         *
//===- include/pstore/core/hamt_map_fwd.hpp -------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file hamt_map_fwd.hpp

#ifndef PSTORE_CORE_HAMT_MAP_FWD_HPP
#define PSTORE_CORE_HAMT_MAP_FWD_HPP

#include <functional>

namespace pstore {
    namespace index {

        /// This class provides a common base from which each of the real index types derives. This
        /// avoids the lower-level storage code needing to know about the types that these indices
        /// contain.
        class index_base {
        public:
            virtual ~index_base () = 0;
        };

        /// The begin() and end() functions for both hamt_map and hamt_set take an extra parameter
        /// -- the owning database -- which prevents the container's direct use in range-based for
        /// loops. This class can provide the required argument. It is created by calling the
        /// make_range() method of either of those container.

        template <typename Database, typename Container, typename Iterator>
        class range {
        public:
            range (Database & d, Container & c)
                    : db_{d}
                    , c_{c} {}
            /// Returns an iterator to the beginning of the container
            Iterator begin () const { return c_.begin (db_); }
            /// Returns an iterator to the end of the container
            Iterator end () const { return c_.end (db_); }

        private:
            Database & db_;
            Container & c_;
        };

        template <typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>,
                  typename KeyEqual = std::equal_to<KeyType>>
        class hamt_map;

        template <typename KeyType, typename Hash = std::hash<KeyType>,
                  typename KeyEqual = std::equal_to<KeyType>>
        class hamt_set;

    } // namespace index
} // namespace pstore
#endif // PSTORE_CORE_HAMT_MAP_FWD_HPP
