//*  _   _                 *
//* | |_(_)_ __ ___   ___  *
//* | __| | '_ ` _ \ / _ \ *
//* | |_| | | | | | |  __/ *
//*  \__|_|_| |_| |_|\___| *
//*                        *
//===- lib/os/time.cpp ----------------------------------------------------===//
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
        assert (res == &result);
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
        assert (res == &result);
        return result;
    }
#else
#    error "Need gmtime_r() or gmtime_s()"
#endif

} // end namespace pstore
