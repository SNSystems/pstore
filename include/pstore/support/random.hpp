//*                      _                  *
//*  _ __ __ _ _ __   __| | ___  _ __ ___   *
//* | '__/ _` | '_ \ / _` |/ _ \| '_ ` _ \  *
//* | | | (_| | | | | (_| | (_) | | | | | | *
//* |_|  \__,_|_| |_|\__,_|\___/|_| |_| |_| *
//*                                         *
//===- include/pstore/support/random.hpp ----------------------------------===//
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
/// \file random.hpp

#ifndef PSTORE_RANDOM_HPP
#define PSTORE_RANDOM_HPP

#include <limits>
#include <random>

namespace pstore {

    template <typename Ty>
    class random_generator {
    public:
        random_generator ()
                : generator_ (device_ ()) {}

        Ty get (Ty max) { return distribution_ (generator_) % max; }
        Ty get () {
            auto const max = std::numeric_limits<Ty>::max ();
            static_assert (max > Ty (0), "max must be > 0");
            return get (max);
        }

    private:
        std::random_device device_;
        std::mt19937_64 generator_;
        std::uniform_int_distribution<Ty> distribution_;
    };

} // namespace pstore

#endif // PSTORE_RANDOM_HPP
// eof: include/pstore/support/random.hpp
