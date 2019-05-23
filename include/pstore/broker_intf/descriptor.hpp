//*      _                     _       _              *
//*   __| | ___  ___  ___ _ __(_)_ __ | |_ ___  _ __  *
//*  / _` |/ _ \/ __|/ __| '__| | '_ \| __/ _ \| '__| *
//* | (_| |  __/\__ \ (__| |  | | |_) | || (_) | |    *
//*  \__,_|\___||___/\___|_|  |_| .__/ \__\___/|_|    *
//*                             |_|                   *
//===- include/pstore/broker_intf/descriptor.hpp --------------------------===//
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

#ifndef PSTORE_BROKER_INTF_DESCRIPTOR_HPP
#define PSTORE_BROKER_INTF_DESCRIPTOR_HPP

#include <cerrno>
#include <ostream>

#ifndef _WIN32
#    include <unistd.h>
#else
#    include <io.h>
#    include <Winsock2.h>
#endif

namespace pstore {
    namespace broker {
        namespace details {

            /// DescriptorTraits is a traits structure of the following form:
            ///
            ///     struct descriptor_traits {
            ///         using type = ...;
            ///         using error_type = ...;
            ///         std::function <bool (type)> is_valid = ...;
            ///         std::function <void (type)> close = ...;
            ///         static constexpr type const invalid = ...;
            ///         static constexpr int const error = ...;
            ///     };
            ///
            /// (The use of std::function<> is for exposition only: the implementation simply
            /// provides a function with a similar signature.)
            ///
            /// - type: the type of a descriptor.
            /// - error_type: the type of the error return value for APIs using the descriptor type.
            /// - is_valid: given a descriptor value, this function returns true if the value is
            /// valid or false if it represents an error condition.
            /// - close: Closes the given descriptor.
            /// - invalid: The value that can be assigned to create an "invalid" descriptor.
            /// - error: The value that can be used to signify an error return code.
            template <typename DescriptorTraits>
            class descriptor {
            public:
                using value_type = typename DescriptorTraits::type;
                using error_type = typename DescriptorTraits::error_type;

#if defined(_WIN32) && defined(_MSC_VER)
                // MSVC doesn't consider INVALID_HANDLE to be integral and won't allow it to be
                // constexpr.
                static value_type const invalid;
                static error_type const error;
#else
                static constexpr value_type invalid = DescriptorTraits::invalid;
                static constexpr error_type error = DescriptorTraits::error;
#endif

                explicit constexpr descriptor (
                    DescriptorTraits traits = DescriptorTraits ()) noexcept
                        : fd_{invalid}
                        , traits_ (traits) {}
                explicit constexpr descriptor (
                    value_type fd, DescriptorTraits traits = DescriptorTraits ()) noexcept
                        : fd_{fd}
                        , traits_ (traits) {}
                descriptor (descriptor && rhs) noexcept
                        : fd_{rhs.release ()}
                        , traits_(std::move (rhs.traits_)) {}
                descriptor (descriptor const &) = delete;

                ~descriptor () {
                    auto const err = errno;
                    reset ();
                    errno = err;
                }

                descriptor & operator= (descriptor const &) = delete;
                descriptor & operator= (descriptor && rhs) noexcept {
                    if (this != &rhs) {
                        reset ();
                        fd_ = rhs.release ();
                        traits_ = std::move (rhs.traits_);
                    }
                    return *this;
                }

                bool operator== (descriptor const & rhs) const noexcept { return fd_ == rhs.fd_; }
                bool operator!= (descriptor const & rhs) const noexcept {
                    return !operator== (rhs);
                }
                bool operator< (descriptor const & rhs) const noexcept { return fd_ < rhs.fd_; }

                bool valid () const noexcept { return traits_.is_valid (fd_); }
                value_type native_handle () const noexcept { return fd_; }

                value_type release () noexcept {
                    auto const r = fd_;
                    fd_ = invalid;
                    return r;
                }

                void reset (value_type r = invalid) noexcept {
                    if (valid ()) {
                        traits_.close (fd_);
                    }
                    fd_ = r;
                }

            private:
                value_type fd_;
                DescriptorTraits traits_;
            };

#if defined(_WIN32) && defined(_MSC_VER)
            template <typename DescriptorTraits>
            typename descriptor<DescriptorTraits>::value_type const
                descriptor<DescriptorTraits>::invalid = DescriptorTraits::invalid;

            template <typename DescriptorTraits>
            typename descriptor<DescriptorTraits>::error_type const
                descriptor<DescriptorTraits>::error = DescriptorTraits::error;
#endif

            template <typename DescriptorTraits>
            inline std::ostream & operator<< (std::ostream & os,
                                              descriptor<DescriptorTraits> const & fd) {
                return os << fd.get ();
            }

#ifndef _WIN32

            // Not Windows.
            class posix_descriptor_traits {
            public:
                using type = int;
                using error_type = type;

                static bool is_valid (type fd) noexcept { return fd >= 0; }
                static void close (type fd) noexcept { ::close (fd); }

                static constexpr type const invalid = -1;
                static constexpr error_type const error = -1;
            };
            using socket_descriptor = descriptor<posix_descriptor_traits>;
            using pipe_descriptor = descriptor<posix_descriptor_traits>;

#else

            // Windows.
            class win32_socket_descriptor_traits {
            public:
                using type = SOCKET;
                using error_type = int;

                static bool is_valid (type fd) noexcept { return fd != invalid; }
                static void close (type fd) noexcept { ::closesocket (fd); }

                static constexpr type const invalid = INVALID_SOCKET;
                static constexpr error_type const error = SOCKET_ERROR;
            };
            using socket_descriptor = descriptor<win32_socket_descriptor_traits>;

            class win32_pipe_descriptor_traits {
            public:
                using type = HANDLE;
                using error_type = type;

                static bool is_valid (type h) noexcept { return h != invalid; }
                static void close (type h) noexcept { ::CloseHandle (h); }

                static type const invalid;
                static error_type const error;
            };
            using pipe_descriptor = descriptor<win32_pipe_descriptor_traits>;

#endif

        } // namespace details

        using socket_descriptor = details::socket_descriptor;
        using pipe_descriptor = details::pipe_descriptor;
#ifdef _WIN32
        using unique_handle = pipe_descriptor;
#endif

    } // namespace broker
} // namespace pstore

namespace std {

    template <typename DescriptorTraits>
    struct hash<pstore::broker::details::descriptor<DescriptorTraits>> {
        using argument_type = pstore::broker::details::descriptor<DescriptorTraits>;
        using result_type = std::size_t;

        result_type operator() (argument_type const & s) const noexcept {
            return std::hash<typename argument_type::value_type> () (s.native_handle ());
        }
    };

} // end namespace std

#endif // PSTORE_BROKER_INTF_DESCRIPTOR_HPP
