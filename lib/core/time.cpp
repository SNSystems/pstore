//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===- lib/core/time.cpp --------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pstore/core/time.hpp"

#include <chrono>
#include <limits>

#include "pstore/support/assert.hpp"

namespace {

    template <typename T, typename U = T>
    constexpr auto type_max () -> U {
        return static_cast<U> (std::numeric_limits<T>::max ());
    }

} // end anonymous namespace

namespace pstore {

    std::uint64_t milliseconds_since_epoch () {
        using namespace std::chrono;
        system_clock::duration const dur = system_clock::now ().time_since_epoch ();
        auto const ms = duration_cast<milliseconds> (dur);

        using rep = milliseconds::rep;
        using urep = std::make_unsigned<rep>::type;

        rep const result = ms.count ();
        PSTORE_ASSERT (result >= 0);

        static_assert (type_max<rep> () >= 0 &&
                           type_max<rep, urep> () <= type_max<std::uint64_t> (),
                       "millisecond representation is too wide for uint64");
        return static_cast<std::uint64_t> (result);
    }

} // namespace pstore
