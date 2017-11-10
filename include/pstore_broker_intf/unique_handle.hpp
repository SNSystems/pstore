//*              _                    _                     _ _       *
//*  _   _ _ __ (_) __ _ _   _  ___  | |__   __ _ _ __   __| | | ___  *
//* | | | | '_ \| |/ _` | | | |/ _ \ | '_ \ / _` | '_ \ / _` | |/ _ \ *
//* | |_| | | | | | (_| | |_| |  __/ | | | | (_| | | | | (_| | |  __/ *
//*  \__,_|_| |_|_|\__, |\__,_|\___| |_| |_|\__,_|_| |_|\__,_|_|\___| *
//*                   |_|                                             *
//===- include/pstore_broker_intf/unique_handle.hpp -----------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc.
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
/// \file unique_handle.hpp

#ifndef PSTORE_UNIQUE_HANDLE_HPP
#define PSTORE_UNIQUE_HANDLE_HPP

#include <utility>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace pstore {
    namespace broker {

        template <typename T, T bad_value, typename Deleter>
        class unique_value {
        public:
            using value_type = T;

            unique_value () = default;
            unique_value (T fd, Deleter d = Deleter ())
                    : value_{fd}
                    , delete_{d} {}
            unique_value (unique_value const &) = delete;
            unique_value (unique_value && rhs) noexcept
                    : value_{rhs.release ()}
                    , delete_{std::move (rhs.delete_)} {}
            ~unique_value () noexcept {
                reset ();
            }
            unique_value & operator= (unique_value const &) = delete;
            unique_value & operator= (unique_value && rhs) noexcept {
                reset (rhs.release ());
                delete_ = std::move (rhs.delete_);
                return *this;
            }
            T get () const noexcept {
                return value_;
            }
            void reset (T d = bad_value) noexcept {
                if (value_ != bad_value) {
                    delete_ (value_);
                }
                value_ = d;
            }
            T release () noexcept {
                auto const res = value_;
                value_ = bad_value;
                return res;
            }
            bool valid () const noexcept {
                return value_ != bad_value;
            }

        private:
            T value_ = bad_value;
            Deleter delete_ = Deleter ();
        };

#ifdef _WIN32
        struct handle_deleter {
            void operator() (HANDLE h) {
                ::CloseHandle (h);
            }
        };

        using unique_handle = unique_value<HANDLE, INVALID_HANDLE_VALUE, handle_deleter>;
        inline unique_handle make_handle (HANDLE fd) {
            return {fd};
        }
#else
        struct fd_deleter {
            void operator() (int fd) {
                ::close (fd);
            }
        };
        using unique_fd = unique_value<int, -1, fd_deleter>;
        inline unique_fd make_fd (int fd) {
            return {fd};
        }
#endif //_WIN32

    } // namespace broker
} // namespace pstore

#endif // PSTORE_UNIQUE_HANDLE_HPP
// eof: include/pstore_broker_intf/unique_handle.hpp
