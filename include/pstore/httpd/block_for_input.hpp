//*  _     _            _       __              _                   _    *
//* | |__ | | ___   ___| | __  / _| ___  _ __  (_)_ __  _ __  _   _| |_  *
//* | '_ \| |/ _ \ / __| |/ / | |_ / _ \| '__| | | '_ \| '_ \| | | | __| *
//* | |_) | | (_) | (__|   <  |  _| (_) | |    | | | | | |_) | |_| | |_  *
//* |_.__/|_|\___/ \___|_|\_\ |_|  \___/|_|    |_|_| |_| .__/ \__,_|\__| *
//*                                                    |_|               *
//===- include/pstore/httpd/block_for_input.hpp ---------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
#ifndef PSTORE_HTTPD_BLOCK_FOR_INPUT_HPP
#define PSTORE_HTTPD_BLOCK_FOR_INPUT_HPP

#include <algorithm>
#include <memory>

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <sys/select.h>
#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/support/logging.hpp"

namespace pstore {
    namespace httpd {

        struct inputs_ready {
            inputs_ready (bool s, bool c)
                    : socket{s}
                    , cv{c} {}
            inputs_ready (inputs_ready const &) = default;
            inputs_ready (inputs_ready &&) = default;
            inputs_ready & operator= (inputs_ready const &) = default;
            inputs_ready & operator= (inputs_ready &&) = default;

            bool socket;
            bool cv;
        };

        // Watch fd to be notified when it has input.
        template <typename Reader>
        inputs_ready block_for_input (Reader const & reader,
                                      broker::socket_descriptor const & socket_fd,
                                      broker::pipe_descriptor const * const cv_fd) {
            // If the reader has data buffered, then we won't block.
            if (reader.available () > 0) {
                return {true, false};
            }

            constexpr auto timeout_seconds = 60L;

#ifdef _WIN32
            std::unique_ptr<std::remove_pointer<WSAEVENT>::type, decltype (&::WSACloseEvent)> event{
                ::WSACreateEvent (), &::WSACloseEvent};
            if (::WSAEventSelect (socket_fd.native_handle (), event.get (), FD_READ | FD_CLOSE) !=
                0) {
                // WSAGetLastError ()
            }

            std::array<WSAEVENT, 2> events;
            auto size = DWORD{0};
            events[size++] = event.get ();
            if (cv_fd) {
                events[size++] = cv_fd->native_handle ();
            }

            for (;;) {
                auto const cause =
                    ::WSAWaitForMultipleEvents (size, events.data (),
                                                FALSE, // Wait for any event to be signaled
                                                timeout_seconds * 1000, // The time-out interval
                                                TRUE                    // Alertable?
                    );
                if (cause == WSA_WAIT_IO_COMPLETION) {
                    continue;
                } else if (cause == WSA_WAIT_TIMEOUT) {
                    log (logging::priority::notice, "wait timeout");
                    return {false, false};
                } else if (cause >= WSA_WAIT_EVENT_0) {
                    auto const index = cause - WSA_WAIT_EVENT_0;
                    return {(index == 0), (index == 1)};
                } else {
                    raise (win32_erc (::GetLastError ()), "WSAWaitForMultipleEvents");
                }
            }
#else
            timeval timeout{timeout_seconds, 0};
            fd_set read_fds;
            FD_ZERO (&read_fds);
            FD_SET (socket_fd.native_handle (), &read_fds);
            if (cv_fd != nullptr) {
                FD_SET (cv_fd->native_handle (), &read_fds);
            }

            fd_set error_fds;
            FD_SET (socket_fd.native_handle (), &error_fds);
            if (cv_fd != nullptr) {
                FD_SET (cv_fd->native_handle (), &error_fds);
            }

            int const maxfd = std::max (socket_fd.native_handle (),
                                        (cv_fd != nullptr) ? cv_fd->native_handle () : 0);
            auto err = 0;
            while ((err = ::select (maxfd + 1, &read_fds, nullptr, &error_fds, &timeout)) == -1 &&
                   errno == EINTR) {
                continue; // Restart if interrupted by signal.
            }

            if (err == -1) {
                raise (errno_erc{errno}, "select");
            } else if (err == 0) {
                log (logging::priority::notice, "no data within timeout");
            }

            auto const isset = [&read_fds, &error_fds](int fd) {
                return FD_ISSET (fd, &read_fds) || FD_ISSET (fd, &error_fds);
            };
            return {isset (socket_fd.native_handle ()),
                    cv_fd != nullptr ? isset (cv_fd->native_handle ()) : false};
#endif
        } // namespace httpd


        // FIXME: this is only here for the unit tests. It shouldn't be.
        template <typename Reader>
        inputs_ready block_for_input (Reader const &, int, broker::pipe_descriptor const *) {
            return {true, false};
        }



    } // namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_BLOCK_FOR_INPUT_HPP
