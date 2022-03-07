//===- lib/support/utf_win32.cpp ------------------------------------------===//
//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file utf_win32.cpp

#include "pstore/support/utf.hpp"

#ifdef _WIN32

// Standard Library includes
#    include <cassert>
#    include <cwchar>
#    include <vector>

// System includes
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

// Local includes
#    include "pstore/support/error.hpp"
#    include "pstore/support/portab.hpp"

namespace {


    int conversion_result (char const * api, int result) {
        if (result == 0) {
            raise (pstore::win32_erc (::GetLastError ()), api);
        }
        // The Win32 conversion functions [MultiByteToWideChar() and WideCharToMultiByte()] both
        // return a signed type, but MS don't document any legal negative return value. If that
        // happens, I'll just assume an empty string.
        return std::max (result, 0);
    }

    inline int length_as_int (std::size_t length) {
        if (length > static_cast<unsigned> (std::numeric_limits<int>::max ())) {
            raise (pstore::errno_erc{EINVAL}, "string was too long for conversion");
        }
        return static_cast<int> (length);
    }

    //*  ___     __    _  __  *
    //* ( _ )  __\ \  / |/ /  *
    //* / _ \ |___> > | / _ \ *
    //* \___/    /_/  |_\___/ *
    //*                       *
    struct from_8_to_16_traits {
        using input_char_type = char;
        using output_char_type = wchar_t;

        static std::size_t output_size (input_char_type const * wstr, std::size_t length);
        static void convert (input_char_type const * str, std::size_t str_length,
                             std::vector<output_char_type> * const output);
    };

    std::size_t from_8_to_16_traits::output_size (input_char_type const * str, std::size_t length) {
        return conversion_result (
            "MultiByteToWideChar",
            ::MultiByteToWideChar (
                CP_UTF8,                // the input string encoding
                0,                      // flags
                str,                    // the UTF-8 encoded input string
                length_as_int (length), // number of UTF-8 code units (bytes) in the input string
                nullptr,                // no output buffer
                0)                      // no output buffer
        );
    }

    void from_8_to_16_traits::convert (input_char_type const * wstr, std::size_t wstr_length,
                                       std::vector<output_char_type> * const output) {

        std::size_t const wstr_size = wstr_length * sizeof (input_char_type);
        std::size_t const output_size = output->size ();

        int const wchars_written = conversion_result (
            "MultiByteToWideChar",
            ::MultiByteToWideChar (
                CP_UTF8, 0,
                wstr,                        // input
                length_as_int (wstr_size),   // length (in bytes) of wstr
                output->data (),             // output buffer
                length_as_int (output_size)) // size (in code units) of the output buffer
        );
        (void) wchars_written;
        PSTORE_ASSERT (wchars_written <= output_size * sizeof (output_char_type));
        PSTORE_ASSERT (wchars_written == from_8_to_16_traits::output_size (wstr, wstr_length));
    }


    //*  _  __     __    ___  *
    //* / |/ /   __\ \  ( _ ) *
    //* | / _ \ |___> > / _ \ *
    //* |_\___/    /_/  \___/ *
    //*                       *
    struct from_16_to_8_traits {
        using input_char_type = wchar_t;
        using output_char_type = char;

        static std::size_t output_size (input_char_type const * wstr, std::size_t length);

        static void convert (input_char_type const * wstr, std::size_t wstr_length,
                             std::vector<output_char_type> * const output);
    };

    std::size_t from_16_to_8_traits::output_size (input_char_type const * wstr,
                                                  std::size_t length) {
        return conversion_result (
            "WideCharToMultiByte",
            ::WideCharToMultiByte (CP_UTF8,                // output encoding
                                   0,                      // flags
                                   wstr,                   // input UTF-16 string
                                   length_as_int (length), // number of UTF-16 elements
                                   nullptr,                // no output buffer
                                   0,                      // output buffer length
                                   nullptr, // missing character (must be NULL for UTF8)
                                   nullptr) // missing character was used? (must be NULL for UTF8)
        );
    }

    void from_16_to_8_traits::convert (input_char_type const * wstr, std::size_t wstr_length,
                                       std::vector<output_char_type> * const output) {
        std::size_t const output_size = output->size () * sizeof (output_char_type);
        int const bytes_written = conversion_result (
            "WideCharToMultiByte",
            ::WideCharToMultiByte (CP_UTF8,                     // output encoding
                                   0,                           // flags
                                   wstr,                        // input UTF-16 string
                                   length_as_int (wstr_length), // number of UTF-16 code-units
                                   output->data (),             // output buffer
                                   length_as_int (output_size), // output buffer length (in bytes)
                                   nullptr, // missing character (must be NULL for UTF8)
                                   nullptr) // missing character was used? (must be NULL for UTF8)
        );
        (void) bytes_written;
        PSTORE_ASSERT (bytes_written <= output_size);
        PSTORE_ASSERT (bytes_written == from_16_to_8_traits::output_size (wstr, wstr_length));
    }

} // end anonymous namespace

namespace {

    template <typename ConverterTraits>
    struct converter {
        using input_char_type = typename ConverterTraits::input_char_type;
        using output_char_type = typename ConverterTraits::output_char_type;

        using input_string_type = std::basic_string<input_char_type>;
        using output_string_type = std::basic_string<output_char_type>;

        output_string_type operator() (input_char_type const * str, std::size_t length);
    };

    // TODO: this code should be more efficient. Rather than computing the actual size of the
    // output buffer that is required then allocating a buffer and finally perform the conversion,
    // it should allocate a default buffer, try the conversion and retry it if the buffer
    // wasn't large enough. That should result in only having a single system call in the
    // typical case.
    template <typename ConverterTraits>
    auto converter<ConverterTraits>::operator() (input_char_type const * str, std::size_t length)
        -> output_string_type {
        output_string_type converted;

        if (str != nullptr && length > 0) {
            std::size_t const chars_required = ConverterTraits::output_size (str, length);
            if (chars_required > 0) {
                std::vector<output_char_type> buffer (chars_required);
                ConverterTraits::convert (str, length, &buffer);

                auto begin = std::begin (buffer);
                converted.assign (begin, begin + chars_required);
            }
        }

        return converted;
    }
} // namespace

namespace pstore {
    namespace utf {
        namespace win32 {
            // to8
            // ~~~
            std::string to8 (wchar_t const * const wstr, std::size_t length) {
                return converter<from_16_to_8_traits> () (wstr, length);
            }

            std::string to8 (wchar_t const * const wstr) {
                std::string converted;
                if (wstr != nullptr && wstr[0] != '\0') {
                    converted = to8 (wstr, std::wcslen (wstr));
                }
                return converted;
            }

            // to16
            // ~~~~
            std::wstring to16 (char const * str, std::size_t length) {
                return converter<from_8_to_16_traits> () (str, length);
            }

            std::wstring to16 (char const * const str) {
                std::wstring converted;
                if (str != nullptr && str[0] != '\0') {
                    converted = to16 (str, std::strlen (str));
                }
                return converted;
            }

            // to_mbcs
            // ~~~~~~~
            std::string to_mbcs (wchar_t const * utf16, std::size_t length) {
                if (length == std::size_t{0}) {
                    return {};
                }
                auto const input_length = static_cast<int> (
                    std::min (length, static_cast<std::size_t> (std::numeric_limits<int>::max ())));
                int size_needed = ::WideCharToMultiByte (CP_ACP, 0, &utf16[0], input_length,
                                                         nullptr, 0, nullptr, nullptr);
                if (size_needed == 0) {
                    raise (win32_erc{::GetLastError ()}, "WideCharToMultiByte");
                }
                std::string str_to (size_needed, '\0');
                size_needed = ::WideCharToMultiByte (CP_ACP, 0, &utf16[0], input_length, &str_to[0],
                                                     size_needed, nullptr, nullptr);
                if (size_needed == 0) {
                    raise (win32_erc{::GetLastError ()}, "WideCharToMultiByte");
                }
                return str_to;
            }

            std::string to_mbcs (char const * str, std::size_t length) {
                if (length == std::size_t{0}) {
                    return {};
                }
                return to_mbcs (to16 (str, length));
            }

            /// Unfortunately, the Windows API forces us to do this conversion in two phases.
            /// First, we must convert the multi-byte character string to UTF-16, then we can
            /// can convert the UTF-16 string to UTF-8.
            std::string mbcs_to8 (char const * mbcs, std::size_t length) {
                if (length == 0) {
                    return {};
                }

                // Find out the number of wchars that the conversion will produce.
                using common = std::common_type<int, std::size_t>::type;
                auto const input_length =
                    std::min (static_cast<common> (length),
                              static_cast<common> (std::numeric_limits<int>::max ()));
                int size_needed =
                    ::MultiByteToWideChar (CP_ACP, MB_ERR_INVALID_CHARS, mbcs,
                                           static_cast<int> (input_length), nullptr, 0);
                if (size_needed == 0) {
                    raise (pstore::win32_erc (::GetLastError ()), "MultiByteToWideChar");
                }

                // Now allocate a wstring string with enough capacity to hold the UTF-16 output
                // string
                std::wstring wstr_to (size_needed, L'\0');
                size_needed = ::MultiByteToWideChar (CP_ACP, MB_ERR_INVALID_CHARS, mbcs,
                                                     static_cast<int> (input_length), &wstr_to[0],
                                                     size_needed);
                if (size_needed == 0) {
                    raise (pstore::win32_erc (::GetLastError ()), "MultiByteToWideChar");
                }

                // Finally, convert the UTF-16 string to UTF-8.
                return to8 (wstr_to);
            }

        } // namespace win32
    }     // namespace utf
} // namespace pstore

#endif // _WIN32
