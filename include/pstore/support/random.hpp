//*                      _                  *
//*  _ __ __ _ _ __   __| | ___  _ __ ___   *
//* | '__/ _` | '_ \ / _` |/ _ \| '_ ` _ \  *
//* | | | (_| | | | | (_| | (_) | | | | | | *
//* |_|  \__,_|_| |_|\__,_|\___/|_| |_| |_| *
//*                                         *
//===- include/pstore/support/random.hpp ----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file random.hpp

#ifndef PSTORE_SUPPORT_RANDOM_HPP
#define PSTORE_SUPPORT_RANDOM_HPP

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

#endif // PSTORE_SUPPORT_RANDOM_HPP
