//===- include/pstore/http/block_for_input.hpp ------------*- mode: C++ -*-===//
//*  _     _            _       __              _                   _    *
//* | |__ | | ___   ___| | __  / _| ___  _ __  (_)_ __  _ __  _   _| |_  *
//* | '_ \| |/ _ \ / __| |/ / | |_ / _ \| '__| | | '_ \| '_ \| | | | __| *
//* | |_) | | (_) | (__|   <  |  _| (_) | |    | | | | | |_) | |_| | |_  *
//* |_.__/|_|\___/ \___|_|\_\ |_|  \___/|_|    |_|_| |_| .__/ \__,_|\__| *
//*                                                    |_|               *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file block_for_input.hpp

#ifndef PSTORE_HTTP_BLOCK_FOR_INPUT_HPP
#define PSTORE_HTTP_BLOCK_FOR_INPUT_HPP

#include <algorithm>
#include <cstring>
#include <memory>

#ifdef _WIN32
#    include <winsock2.h>
#else
#    include <sys/select.h>
#endif

#include "pstore/os/descriptor.hpp"
#include "pstore/os/logging.hpp"

namespace pstore {
    namespace http {

        struct inputs_ready {
            constexpr inputs_ready (bool const s, bool const c) noexcept
                    : socket{s}
                    , cv{c} {}

            /// True if data is available on the input socket.
            bool socket;
            /// True if a condition variable has been signalled.
            bool cv;
        };

        // Watch fd to be notified when it has input.
        template <typename Reader>
        inputs_ready block_for_input (Reader const & reader, socket_descriptor const & socket_fd,
                                      pipe_descriptor const * const cv_fd) {
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
                    log (logger::priority::notice, "wait timeout");
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
            FD_ZERO (&read_fds);                            // NOLINT
            FD_SET (socket_fd.native_handle (), &read_fds); // NOLINT
            if (cv_fd != nullptr) {
                FD_SET (cv_fd->native_handle (), &read_fds); // NOLINT
            }

            fd_set error_fds;
            FD_ZERO (&error_fds);                            // NOLINT
            FD_SET (socket_fd.native_handle (), &error_fds); // NOLINT
            if (cv_fd != nullptr) {
                FD_SET (cv_fd->native_handle (), &error_fds); // NOLINT
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
                log (logger::priority::notice, "no data within timeout");
            }

            auto const isset = [&read_fds, &error_fds] (int const fd) {
                return FD_ISSET (fd, &read_fds) || FD_ISSET (fd, &error_fds);
            };
            return {isset (socket_fd.native_handle ()),
                    cv_fd != nullptr ? isset (cv_fd->native_handle ()) : false};
#endif
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_BLOCK_FOR_INPUT_HPP
