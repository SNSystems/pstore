//*      _                   _               *
//*  ___(_) __ _ _ __   __ _| |   _____   __ *
//* / __| |/ _` | '_ \ / _` | |  / __\ \ / / *
//* \__ \ | (_| | | | | (_| | | | (__ \ V /  *
//* |___/_|\__, |_| |_|\__,_|_|  \___| \_/   *
//*        |___/                             *
//===- lib/os/signal_cv_win32.cpp -----------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
/// \file signal_cv_win32.cpp

#include "pstore/os/signal_cv.hpp"

#ifdef _WIN32

#    include <cassert>
#    include "pstore/support/error.hpp"
#    include "pstore/support/scope_guard.hpp"

namespace pstore {

    //*     _                _      _              _____   __ *
    //*  __| |___ ___ __ _ _(_)_ __| |_ ___ _ _   / __\ \ / / *
    //* / _` / -_|_-</ _| '_| | '_ \  _/ _ \ '_| | (__ \ V /  *
    //* \__,_\___/__/\__|_| |_| .__/\__\___/_|    \___| \_/   *
    //*                       |_|                             *
    // (ctor)
    // ~~~~~~
    descriptor_condition_variable::descriptor_condition_variable ()
            : event_{::CreateEvent (nullptr /*security attributes*/, FALSE /*manual reset?*/,
                                    FALSE /*initially signalled?*/, nullptr /*name*/)} {
        if (event_.native_handle () == nullptr) {
            raise (win32_erc (::GetLastError ()), "CreateEvent");
        }
    }

    // wait_descriptor
    // ~~~~~~~~~~~~~~~
    pipe_descriptor const & descriptor_condition_variable::wait_descriptor () const noexcept {
        return event_;
    }

    // notify_all
    // ~~~~~~~~~~
    void descriptor_condition_variable::notify_all () {
        if (!::SetEvent (event_.native_handle ())) {
            raise (win32_erc{::GetLastError ()}, "SetEvent");
        }
    }

    // notify_all_no_except
    // ~~~~~~~~~~~~~~~~~~~~
    void descriptor_condition_variable::notify_all_no_except () noexcept {
        ::SetEvent (event_.native_handle ());
    }

    // wait
    // ~~~~
    void descriptor_condition_variable::wait () {
        switch (::WaitForSingleObject (this->wait_descriptor ().native_handle (), INFINITE)) {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0: return;
        case WAIT_TIMEOUT: PSTORE_ASSERT (0);
        // fallthrough
        case WAIT_FAILED: raise (win32_erc (::GetLastError ()), "WaitForSingleObject");
        }
    }

    void descriptor_condition_variable::wait (std::unique_lock<std::mutex> & lock) {
        lock.unlock ();
        auto _ = make_scope_guard ([&lock] () { lock.lock (); });
        this->wait ();
    }

    // reset
    // ~~~~~
    void descriptor_condition_variable::reset () { ::ResetEvent (event_.native_handle ()); }

} // end namespace pstore

#endif //_WIN32
