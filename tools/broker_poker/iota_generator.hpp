//*  _       _                                          _              *
//* (_) ___ | |_ __ _    __ _  ___ _ __   ___ _ __ __ _| |_ ___  _ __  *
//* | |/ _ \| __/ _` |  / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \| '__| *
//* | | (_) | || (_| | | (_| |  __/ | | |  __/ | | (_| | || (_) | |    *
//* |_|\___/ \__\__,_|  \__, |\___|_| |_|\___|_|  \__,_|\__\___/|_|    *
//*                     |___/                                          *
//===- tools/broker_poker/iota_generator.hpp ------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef PSTORE_BROKER_POKER_IOTA_GENERATOR_HPP
#define PSTORE_BROKER_POKER_IOTA_GENERATOR_HPP

#include <iterator>

/// An InputIterator which will generate a monotonically increasing integer value.
class iota_generator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = unsigned long;
    using difference_type = std::ptrdiff_t;
    using pointer = unsigned long *;
    using reference = unsigned long &;

    explicit iota_generator (value_type const v = 0UL)
            : v_{v} {}
    iota_generator (iota_generator const & rhs) = default;
    iota_generator & operator= (iota_generator const & rhs) = default;

    bool operator== (iota_generator const & rhs) const noexcept { return v_ == rhs.v_; }
    bool operator!= (iota_generator const & rhs) const noexcept { return !operator== (rhs); }

    iota_generator & operator++ () {
        ++v_;
        return *this;
    }
    iota_generator operator++ (int) {
        auto const t = *this;
        ++(*this);
        return t;
    }

    value_type operator* () const noexcept { return v_; }
    // pointer operator->() const noexcept { return &v_; }

private:
    value_type v_;
};


#endif // PSTORE_BROKER_POKER_IOTA_GENERATOR_HPP
