//*      _                   _   _          _                      *
//*  ___(_) __ _ _ __   __ _| | | |__   ___| |_ __   ___ _ __ ___  *
//* / __| |/ _` | '_ \ / _` | | | '_ \ / _ \ | '_ \ / _ \ '__/ __| *
//* \__ \ | (_| | | | | (_| | | | | | |  __/ | |_) |  __/ |  \__ \ *
//* |___/_|\__, |_| |_|\__,_|_| |_| |_|\___|_| .__/ \___|_|  |___/ *
//*        |___/                             |_|                   *
//===- include/pstore/support/signal_helpers.hpp --------------------------===//
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

/// \file signal_helpers.hpp

#ifndef PSTORE_SIGNAL_HELPERS_HPP
#define PSTORE_SIGNAL_HELPERS_HPP (1)

#include <errno.h>
#include <signal.h>

namespace pstore {

    // A simple RAII class which preserves the value of errno. It's needed in the signal handler to
    // ensure that we don't splat the value that the in-flight code may depend upon.
    class errno_saver {
    public:
        errno_saver () noexcept
                : old_{errno} {
            errno = 0;
        }
        errno_saver (errno_saver const &) = delete;
        ~errno_saver () noexcept { errno = old_; }
        errno_saver & operator= (errno_saver const &) = delete;

    private:
        int const old_;
    };

    typedef void (*signal_function) (int);
    signal_function register_signal_handler (int signo, signal_function func);

} // namespace pstore
#endif // BROKER_SIGNAL_HELPERS_HPP
// eof: include/pstore/support/signal_helpers.hpp
