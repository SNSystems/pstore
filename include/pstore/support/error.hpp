//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- include/pstore/support/error.hpp -----------------------------------===//
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
/// \file support/error.hpp
/// \brief Provides an pstore-specific error codes and a suitable error category for them.

#ifndef PSTORE_SUPPORT_ERROR_HPP
#define PSTORE_SUPPORT_ERROR_HPP

#include <cstdlib>
#include <string>
#include <system_error>
#include <type_traits>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#endif

#include "pstore/support/portab.hpp"
#include "pstore/support/utf.hpp"

#if !PSTORE_EXCEPTIONS
#    include <iostream>
#endif

namespace pstore {

#define PSTORE_ERROR_CODES                                                                         \
    X (none)                                                                                       \
    X (unknown_revision)                                                                           \
    X (header_corrupt)                                                                             \
    X (header_version_mismatch)                                                                    \
    X (footer_corrupt)                                                                             \
    X (index_corrupt)                                                                              \
    X (bad_alignment) /* an address was not correctly aligned for its pointee type */              \
    X (index_not_latest_revision)                                                                  \
    X (unknown_process_path) /* could not discover the path of the calling process image*/         \
    X (store_closed)         /*an attempt to read or write from a store which is not open*/        \
    X (cannot_allocate_after_commit) /*cannot allocate data after a transaction has been           \
                                        committed*/                                                \
    X (bad_address)       /*an attempt to address an address outside of the allocated storage*/    \
    X (read_only_address) /* an attempt to write to read-only storage*/                            \
    X (did_not_read_number_of_bytes_requested)                                                     \
    X (uuid_parse_error)                                                                           \
    X (bad_message_part_number)                                                                    \
    X (unable_to_open_named_pipe)                                                                  \
    X (pipe_write_timeout)
    // Add more error values here

    // **************
    // * error code *
    // **************
#define X(a) a,
    enum class error_code : int { PSTORE_ERROR_CODES };
#undef X



    // ******************
    // * error category *
    // ******************
    class error_category : public std::error_category {
    public:
        // The need for this constructor was removed by CWG defect 253 but Clang (prior to 3.9.0)
        // and GCC (before 4.6.4) require its presence.
        error_category () noexcept {} // NOLINT
        char const * name () const noexcept override;
        std::string message (int error) const override;
    };

    std::error_category const & get_error_category ();

    inline std::error_code make_error_code (error_code const e) {
        static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                       "base type of pstore::error_code must be int to permit safe static cast");
        return {static_cast<int> (e), get_error_category ()};
    }

    // *************
    // * errno_erc *
    // *************
    /// This class is a tiny wrapper that allows an errno value to be passed to
    /// std::make_error_code().
    class errno_erc {
    public:
        constexpr explicit errno_erc (int const err) noexcept
                : err_{err} {}
        constexpr int get () const noexcept { return err_; }

    private:
        int err_;
    };

    inline std::error_code make_error_code (errno_erc const e) noexcept {
        return {e.get (), std::generic_category ()};
    }

#ifdef _WIN32
    class win32_erc {
    public:
        constexpr explicit win32_erc (DWORD err) noexcept
                : err_{err} {}
        constexpr int get () const noexcept { return err_; }

    private:
        DWORD err_;
    };

    inline std::error_code make_error_code (win32_erc const e) noexcept {
        return {e.get (), std::system_category ()};
    }
#endif //_WIN32

} // end namespace pstore

namespace std {

    template <>
    struct is_error_code_enum<pstore::error_code> : std::true_type {};
    template <>
    struct is_error_code_enum<pstore::errno_erc> : std::true_type {};

#ifdef _WIN32
    template <>
    struct is_error_code_enum<pstore::win32_erc> : std::true_type {};
#endif //_WIN32

} // end namespace std

namespace pstore {

    template <typename Exception, typename = typename std::enable_if<
                                      std::is_base_of<std::exception, Exception>::value>::type>
    PSTORE_NO_RETURN void raise_exception (Exception const & ex) {
#if PSTORE_EXCEPTIONS
        throw ex;
#else
#    ifdef _WIN32
        std::wcerr << L"Error: " << utf::win32::to16 (ex.what ()) << L'\n';
#    else
        std::cerr << "Error: " << ex.what () << '\n';
#    endif
        std::exit (EXIT_FAILURE);
#endif
    }

    template <typename ErrorCode>
    PSTORE_NO_RETURN void raise_error_code (ErrorCode e) {
        raise_exception (std::system_error{e});
    }
    template <typename ErrorCode, typename StrType>
    PSTORE_NO_RETURN void raise_error_code (ErrorCode e, StrType const & what) {
        raise_exception (std::system_error{e, what});
    }

    template <typename ErrorType>
    PSTORE_NO_RETURN void raise (ErrorType e) {
        raise_error_code (make_error_code (e));
    }
    template <typename ErrorType, typename StrType>
    PSTORE_NO_RETURN void raise (ErrorType e, StrType const & what) {
        raise_error_code (make_error_code (e), what);
    }

} // end namespace pstore

#endif // PSTORE_SUPPORT_ERROR_HPP
