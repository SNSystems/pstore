//*                                       __ _ _                                    *
//*  _ __  _ __ ___   ___ ___  ___ ___   / _(_) | ___   _ __   __ _ _ __ ___   ___  *
//* | '_ \| '__/ _ \ / __/ _ \/ __/ __| | |_| | |/ _ \ | '_ \ / _` | '_ ` _ \ / _ \ *
//* | |_) | | | (_) | (_|  __/\__ \__ \ |  _| | |  __/ | | | | (_| | | | | | |  __/ *
//* | .__/|_|  \___/ \___\___||___/___/ |_| |_|_|\___| |_| |_|\__,_|_| |_| |_|\___| *
//* |_|                                                                             *
//===- include/pstore_support/process_file_name.hpp -----------------------===//
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
/// \file process_file_name.hpp
/// \brief Functions to return the path of the current process image.

#ifndef PSTORE_PROCESS_FILE_NAME_HPP
#define PSTORE_PROCESS_FILE_NAME_HPP (1)

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "pstore_support/gsl.hpp"
#include "pstore_support/error.hpp"

namespace pstore {
    /// Returns the path of the current process image.
    std::string process_file_name ();
}


namespace pstore {

    /// \tparam ProcessPathFunction This should be a Callable type with a signature equivalent to
    /// std::function <std::size_t (gsl::span<char>)>.
    /// \tparam BufferType  Should be a vector-like type which supports size(),capacity(),and
    /// resize() and should be convertable to gsl::span<char>.
    ///
    /// \param get_process_path  A function which will be clled to retrieve the current process file
    /// name. It is passed a character span into which the result should be written. The return
    /// value is the number of characters places in that output buffer.
    /// \param buffer A vector-like object which, on return, will contain the process file name.
    /// \result The number of valid characters in the 'buffer' container.
    template <typename ProcessPathFunction, typename BufferType>
    std::size_t process_file_name (ProcessPathFunction get_process_path, BufferType & buffer) {
        static_assert (std::is_same<typename BufferType::value_type, char>::value,
                       "BufferType::value_type must be char");
        constexpr auto const max_reasonable_size = std::size_t{16 * 1024 * 1024};
        auto size = std::size_t{0};
        auto next_size = std::max (buffer.capacity (), std::size_t{2});
        do {
            buffer.resize (next_size);
            size = get_process_path (::pstore::gsl::make_span (buffer));
            next_size = std::max (size, next_size + next_size / 2U);
        } while ((size == 0 || size >= buffer.size ()) && next_size < max_reasonable_size);
        if (next_size >= max_reasonable_size) {
            raise (error_code::unknown_process_path);
        }
        return size;
    }

    namespace freebsd {
        /// \brief Returns the path of the current process on FreeBSD.
        ///
        /// This function is a wrapper around the FreeBSD sysctl() function
        /// which, amongst other things, can be used to discover the path of
        /// a process given its ID.
        ///
        /// \note The function is exposed here to enable unit testing.
        ///
        /// \tparam BufferType  Should be a vector-like type which supports size(),capacity(),and
        /// resize() and should be convertable to gsl::span<char>.
        /// \tparam SysCtlFunction  The sysctl() function or a replacement intended for unit testung.
        ///
        /// \param mib  A command array which tells the sysctl() function what to do.
        /// \param ctl  The real sysctl() function or mock implementation thereof.
        /// \param buffer  A vector-like object which will hold the result of calling the 'ctl'
        /// function.
        /// \result The number of valid characters in the 'buffer' container.
        template <typename SpanType, typename SysCtlFunction, typename BufferType>
        std::size_t process_file_name (SpanType mib, SysCtlFunction ctl, BufferType & buffer) {
            static_assert (std::is_same <typename SpanType::element_type, int>::value,
                           "mib must be a span of integers");

            auto read_link = [&](::pstore::gsl::span<char> b) -> std::size_t {
                assert (b.size () >= 0);
                auto const buffer_size = static_cast<std::size_t> (b.size ());
                auto length = buffer_size;
                errno = 0;
                if (ctl (mib.data (), static_cast<unsigned int> (mib.size ()), b.data (),
                         &length, nullptr, 0) == -1) {
                    if (errno == ENOMEM) {
                        return buffer_size;
                    }
                    raise (errno_erc {errno}, "sysctl(CTL_KERN/KERN_PROC/KERN_PROC_PATHNAME)");
                }
                // Subtract 1 to ignore the terminating null character.
                return std::max (std::size_t{1}, length) - 1U;
            };
            return pstore::process_file_name (read_link, buffer);
        }
    } // namespace freebsd

} // namespace pstore
#endif // PSTORE_PROCESS_FILE_NAME_HPP
// eof: include/pstore_support/process_file_name.hpp
