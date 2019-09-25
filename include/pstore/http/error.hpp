//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- include/pstore/http/error.hpp --------------------------------------===//
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
#ifndef PSTORE_HTTP_ERROR_HPP
#define PSTORE_HTTP_ERROR_HPP

#include <string>
#include <system_error>

#ifdef _WIN32
#    include <winsock2.h>
#endif // _WIN32

#include "pstore/support/error.hpp"


namespace pstore {
    namespace httpd {
        // **************
        // * error code *
        // **************
        enum class error_code : int {
            bad_request = 1,
            bad_websocket_version,
            not_implemented,
            string_too_long,
            refill_out_of_range,
        };

        // ******************
        // * error category *
        // ******************
        class error_category : public std::error_category {
        public:
            // The need for this constructor was removed by CWG defect 253 but Clang (prior
            // to 3.9.0) and GCC (before 4.6.4) require its presence.
            error_category () noexcept {} // NOLINT
            char const * name () const noexcept override;
            std::string message (int error) const override;
        };

        std::error_category const & get_error_category () noexcept;

        inline std::error_code make_error_code (error_code e) {
            static_assert (
                std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                "base type of pstore::httpd::error_code must be int to permit safe static cast");
            return {static_cast<int> (e), get_error_category ()};
        }


        // get_last_error
        // ~~~~~~~~~~~~~~
        inline std::error_code get_last_error () noexcept {
#ifdef _WIN32
            return make_error_code (win32_erc{static_cast<DWORD> (WSAGetLastError ())});
#else
            return make_error_code (errno_erc{errno});
#endif // !_WIN32
        }

    } // namespace httpd
} // namespace pstore


namespace std {

    template <>
    struct is_error_code_enum<pstore::httpd::error_code> : std::true_type {};

} // end namespace std


#endif // PSTORE_HTTP_ERROR_HPP
