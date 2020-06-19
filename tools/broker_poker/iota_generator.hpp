//*  _       _                                          _              *
//* (_) ___ | |_ __ _    __ _  ___ _ __   ___ _ __ __ _| |_ ___  _ __  *
//* | |/ _ \| __/ _` |  / _` |/ _ \ '_ \ / _ \ '__/ _` | __/ _ \| '__| *
//* | | (_) | || (_| | | (_| |  __/ | | |  __/ | | (_| | || (_) | |    *
//* |_|\___/ \__\__,_|  \__, |\___|_| |_|\___|_|  \__,_|\__\___/|_|    *
//*                     |___/                                          *
//===- tools/broker_poker/iota_generator.hpp ------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
