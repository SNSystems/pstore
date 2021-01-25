//*                                  _   _              *
//*   __ _  ___ _ __   ___ _ __ __ _| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__/ _` | __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ | | (_| | |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  \__,_|\__|_|\___/|_| |_| *
//*  |___/                                              *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===- include/pstore/core/generation_iterator.hpp ------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef PSTORE_CORE_GENERATION_ITERATOR_HPP
#define PSTORE_CORE_GENERATION_ITERATOR_HPP

#include <iterator>
#include <tuple>

#include "pstore/core/file_header.hpp"

namespace pstore {
    class database;
    struct trailer;

    class generation_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = typed_address<trailer>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        generation_iterator (gsl::not_null<database const *> const db,
                             typed_address<trailer> const pos)
                : db_{db}
                , pos_{pos} {

            this->validate ();
        }
        generation_iterator (generation_iterator const &) = default;
        generation_iterator (generation_iterator &&) noexcept = default;

        generation_iterator & operator= (generation_iterator const &) = default;
        generation_iterator & operator= (generation_iterator &&) noexcept = default;

        bool operator== (generation_iterator const & rhs) const noexcept {
            return std::tie (db_, pos_) == std::tie (rhs.db_, rhs.pos_);
        }
        bool operator!= (generation_iterator const & rhs) const { return !operator== (rhs); }

        typed_address<trailer> operator* () const { return pos_; }

        generation_iterator & operator++ ();  // pre-increment
        generation_iterator operator++ (int); // post-increment

    private:
        bool validate () const;

        database const * db_;
        typed_address<trailer> pos_;
    };


    class generation_container {
    public:
        constexpr explicit generation_container (database const & db) noexcept
                : db_ (db) {}
        generation_iterator begin ();
        generation_iterator end ();

    private:
        database const & db_;
    };

} // end namespace pstore
#endif // PSTORE_CORE_GENERATION_ITERATOR_HPP
