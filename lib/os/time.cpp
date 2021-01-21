//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===- lib/os/time.cpp ----------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/os/time.hpp"

#include <cassert>
#include <cerrno>

#include "pstore/config/config.hpp"
#include "pstore/support/error.hpp"

namespace pstore {

#ifdef PSTORE_HAVE_LOCALTIME_S
    struct std::tm local_time (std::time_t const clock) {
        struct tm result;
        if (errno_t const err = localtime_s (&result, &clock)) {
            raise (errno_erc{errno}, "localtime_s");
        }
        return result;
    }
#elif defined(PSTORE_HAVE_LOCALTIME_R)
    struct std::tm local_time (std::time_t const clock) {
        errno = 0;
        struct tm result {};
        struct tm * const res = localtime_r (&clock, &result);
        if (res == nullptr) {
            raise (errno_erc{errno}, "localtime_r");
        }
        PSTORE_ASSERT (res == &result);
        return result;
    }
#else
#    error "Need localtime_r() or localtime_s()"
#endif

#ifdef PSTORE_HAVE_GMTIME_S
    struct std::tm gm_time (std::time_t const clock) {
        struct tm result;
        if (errno_t const err = gmtime_s (&result, &clock)) {
            raise (errno_erc{errno}, "gmtime_s");
        }
        return result;
    }
#elif defined(PSTORE_HAVE_GMTIME_R)
    struct std::tm gm_time (std::time_t const clock) {
        errno = 0;
        struct tm result {};
        struct tm * const res = gmtime_r (&clock, &result);
        if (res == nullptr) {
            raise (errno_erc{errno}, "gmtime_r");
        }
        PSTORE_ASSERT (res == &result);
        return result;
    }
#else
#    error "Need gmtime_r() or gmtime_s()"
#endif

} // end namespace pstore
