//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- include/pstore/support/utf.hpp -------------------------------------===//
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
/// \file support/utf.hpp
/// \brief Functionality for processing UTF-8 strings. On Windows, provides an
/// additional set of functions to convert UTF-8 strings to and from UTF-16.

#ifndef PSTORE_SUPPORT_UTF_HPP
#define PSTORE_SUPPORT_UTF_HPP (1)

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"
#include "pstore/support/portab.hpp"

#if defined(_WIN32)

#include <tchar.h>

namespace pstore {
    namespace utf {
        namespace win32 {

            /// Converts an array of UTF-16 encoded wchar_t to UTF-8.
            /// \param wstr    The buffer start address.
            /// \param length  The number of bytes in the buffer.
            /// \return A UTF-8 encoded string equivalent to the string given by the buffer starting
            ///  at wstr.
            std::string to8 (wchar_t const * const wstr, std::size_t length);

            /// Converts a NUL-terminated array of wchar_t from UTF-16 to UTF-8.
            std::string to8 (wchar_t const * const wstr);

            /// Converts a UTF-16 encoded wstring to UTF-8.
            inline std::string to8 (std::wstring const & wstr) {
                return to8 (wstr.data (), wstr.length ());
            }

            /// Converts an array of UTF-8 encoded chars to UTF-16.
            /// \param str     The buffer start address.
            /// \param length  The number of bytes in the buffer.
            /// \return A UTF-16 encoded string equivalent to the string given by the buffer
            /// starting at str.
            std::wstring to16 (char const * str, std::size_t length);

            /// Converts a null-terminated string of UTF-8 encoded chars to UTF-16.
            std::wstring to16 (char const * str);

            inline std::wstring to16 (std::string const & str) {
                return to16 (str.data (), str.length ());
            }

            ///@{
            /// Converts a UTF-8 encoded char sequence to a "multi-byte" string using
            /// the system default Windows ANSI code page.
            /// \param length The number of bytes in the 'str' buffer.
            std::string to_mbcs (char const * str, std::size_t length);
            inline std::string to_mbcs (char const * str) {
                assert (str != nullptr);
                return to_mbcs (str, std::strlen (str));
            }
            inline std::string to_mbcs (std::string const & str) {
                return to_mbcs (str.data (), str.length ());
            }
            std::string to_mbcs (wchar_t const * wstr, std::size_t length);
            inline std::string to_mbcs (std::wstring const & str) {
                return to_mbcs (str.data (), str.length ());
            }
            ///@}

            ///@{
            /// Converts a Windows "multi-byte" character string using the system default
            /// ANSI code page to UTF-8. Note that the system code page can vary from one
            /// machine to another, so such strings should only ever be temporary.

            /// \param mbcs A buffer containing an array of "multi-byte" characters to be converted
            ///             to UTF-8. If must contain the number of bytes given by the 'length'
            ///             parameter.
            /// \param length The number of bytes in the 'mbcs' buffer.
            std::string mbcs_to8 (char const * mbcs, std::size_t length);

            /// \param str A null-terminating string of "multi-byte" characters to be converted
            ///            to UTF-8.
            inline std::string mbcs_to8 (char const * str) {
                assert (str != nullptr);
                return mbcs_to8 (str, std::strlen (str));
            }

            /// \param str  A string of "multi-byte" characters to be converted to UTF-8.
            inline std::string mbcs_to8 (std::string const & str) {
                return mbcs_to8 (str.data (), str.length ());
            }
            ///@}
        } // namespace win32
    }     // namespace utf
} // namespace pstore

#else //_WIN32

using TCHAR = char;

#endif // !defined(_WIN32)


namespace pstore {
    namespace utf {

        using utf8_string = std::basic_string<std::uint8_t>;
        using utf16_string = std::basic_string<char16_t>;

        std::ostream & operator<< (std::ostream & os, utf8_string const & s);

        class utf8_decoder {
        public:
            maybe<char32_t> get (std::uint8_t c) noexcept;
            bool is_well_formed () const noexcept { return well_formed_; }

        private:
            enum state { accept, reject };

            static std::uint8_t decode (gsl::not_null<std::uint8_t *> state,
                                        gsl::not_null<char32_t *> codep,
                                        std::uint32_t const byte) noexcept;

            static std::uint8_t const utf8d_[];
            char32_t codepoint_ = 0;
            std::uint8_t state_ = accept;
            bool well_formed_ = true;
        };


        constexpr char32_t replacement_char_code_point = 0xFFFD;

        template <typename CharType = char, typename OutputIt>
        OutputIt code_point_to_utf8 (char32_t c, OutputIt out);


        template <typename CharType = char, typename OutputIt>
        OutputIt replacement_char (OutputIt out) {
            return code_point_to_utf8<CharType> (replacement_char_code_point, out);
        }

        template <typename CharType, typename OutputIt>
        OutputIt code_point_to_utf8 (char32_t c, OutputIt out) {
            if (c < 0x80) {
                *(out++) = static_cast<CharType> (c);
            } else {
                if (c < 0x800) {
                    *(out++) = static_cast<CharType> (c / 64U + 0xC0U);
                    *(out++) = static_cast<CharType> (c % 64U + 0x80U);
                } else if (c >= 0xD800 && c < 0xE000) {
                    out = replacement_char<CharType> (out);
                } else if (c < 0x10000) {
                    *(out++) = static_cast<CharType> ((c / 0x1000U) | 0xE0U);
                    *(out++) = static_cast<CharType> ((c / 64U % 64U) | 0x80U);
                    *(out++) = static_cast<CharType> ((c % 64U) | 0x80U);
                } else if (c < 0x110000) {
                    *(out++) = static_cast<CharType> ((c / 0x40000U) | 0xF0U);
                    *(out++) = static_cast<CharType> ((c / 0x1000U % 64U) | 0x80U);
                    *(out++) = static_cast<CharType> ((c / 64U % 64U) | 0x80U);
                    *(out++) = static_cast<CharType> ((c % 64U) | 0x80U);
                } else {
                    out = replacement_char<CharType> (out);
                }
            }
            return out;
        }

        template <typename ResultType>
        ResultType code_point_to_utf8 (char32_t c) {
            ResultType result;
            code_point_to_utf8<typename ResultType::value_type> (c, std::back_inserter (result));
            return result;
        }

        inline constexpr std::uint16_t nop_swapper (std::uint16_t v) noexcept { return v; }
        inline constexpr std::uint16_t byte_swapper (std::uint16_t v) noexcept {
            return static_cast<std::uint16_t> (((v & 0x00FFU) << 8U) | ((v & 0xFF00U) >> 8U));
        }

        inline constexpr bool is_utf16_high_surrogate (std::uint16_t code_unit) noexcept {
            return code_unit >= 0xD800 && code_unit <= 0xDBFF;
        }

        // is_utf16_low_surrogate
        // ~~~~~~~~~~~~~~~~~~~~~~
        inline constexpr bool is_utf16_low_surrogate (std::uint16_t code_unit) noexcept {
            return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
        }

        // utf16_to_code_point
        // ~~~~~~~~~~~~~~~~~~~
        template <typename InputIterator, typename SwapperFunction>
        std::pair<InputIterator, char32_t>
        utf16_to_code_point (InputIterator first, InputIterator last, SwapperFunction swapper) {

            using value_type = typename std::remove_cv<
                typename std::iterator_traits<InputIterator>::value_type>::type;
            static_assert (std::is_same<value_type, char16_t>::value,
                           "iterator must produce char16_t");

            assert (first != last);
            char32_t code_point = 0;
            char16_t code_unit = swapper (*(first++));
            if (!is_utf16_high_surrogate (code_unit)) {
                code_point = code_unit;
            } else {
                if (first == last) {
                    code_point = replacement_char_code_point;
                } else {
                    auto const high = code_unit;
                    auto const low = swapper (*(first++));

                    if (low < 0xDC00 || low > 0xDFFF) {
                        code_point = replacement_char_code_point;
                    } else {
                        code_point = 0x10000;
                        code_point += (high & 0x03FFU) << 10U;
                        code_point += (low & 0x03FFU);
                    }
                }
            }
            return {first, code_point};
        }

        // utf16_to_code_points
        // ~~~~~~~~~~~~~~~~~~~~
        template <typename InputIt, typename OutputIt, typename Swapper>
        OutputIt utf16_to_code_points (InputIt first, InputIt last, OutputIt out, Swapper swapper) {
            while (first != last) {
                char32_t code_point;
                std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
                *(out++) = code_point;
            }
            return out;
        }

        template <typename ResultType, typename InputIt, typename Swapper>
        ResultType utf16_to_code_points (InputIt first, InputIt last, Swapper swapper) {
            ResultType result;
            utf16_to_code_points (first, last, std::back_inserter (result), swapper);
            return result;
        }

        template <typename ResultType, typename InputType, typename Swapper>
        ResultType utf16_to_code_points (InputType const & src, Swapper swapper) {
            return utf16_to_code_points<ResultType> (std::begin (src), std::end (src), swapper);
        }


        // utf16_to_code_point
        // ~~~~~~~~~~~~~~~~~~~
        template <typename InputType, typename Swapper>
        char32_t utf16_to_code_point (InputType const & src, Swapper swapper) {
            auto end = std::end (src);
            char32_t cp;
            std::tie (end, cp) = utf16_to_code_point (std::begin (src), end, swapper);
            assert (end == std::end (src));
            return cp;
        }


        // utf16_to_utf8
        // ~~~~~~~~~~~~~
        template <typename InputIt, typename OutputIt, typename Swapper>
        OutputIt utf16_to_utf8 (InputIt first, InputIt last, OutputIt out, Swapper swapper) {
            while (first != last) {
                char32_t code_point;
                std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
                out = code_unit_to_utf8 (code_point, out);
            }
            return out;
        }

        template <typename ResultType, typename InputIt, typename Swapper>
        ResultType utf16_to_utf8 (InputIt first, InputIt last, Swapper swapper) {
            ResultType result;
            utf16_to_utf8 (first, last, std::back_inserter (result), swapper);
            return result;
        }

        template <typename ResultType, typename InputType, typename Swapper>
        ResultType utf16_to_utf8 (InputType const & src, Swapper swapper) {
            return utf16_to_utf8<ResultType> (std::begin (src), std::end (src), swapper);
        }

    } // namespace utf
} // namespace pstore



namespace pstore {
    namespace utf {
        /// If the top two bits are 0b10, then this is a UTF-8 continuation byte
        /// and is skipped; other patterns in these top two bits represent the
        /// start of a character.
        template <typename CharType>
        inline constexpr bool is_utf_char_start (CharType c) {
            using uchar_type = typename std::make_unsigned<CharType>::type;
            return (static_cast<uchar_type> (c) & 0xC0U) != 0x80U;
        }

        ///@{
        /// Returns the number of UTF-8 code-points in a sequence.

        template <typename Iterator>
        std::size_t length (Iterator first, Iterator last) {
            auto const result =
                std::count_if (first, last, [](char c) { return is_utf_char_start (c); });
            assert (result >= 0);
            using utype = typename std::make_unsigned<decltype (result)>::type;
            static_assert (std::numeric_limits<utype>::max () <=
                               std::numeric_limits<std::size_t>::max (),
                           "std::size_t cannot hold result of count_if");
            return static_cast<std::size_t> (result);
        }

        template <typename SpanType>
        std::size_t length (SpanType span) {
            return length (span.begin (), span.end ());
        }

        /// Returns the number of UTF-8 code points in the buffer given by a start address and
        /// length.
        /// \param str  The buffer start address.
        /// \param nbytes The number of bytes in the buffer.
        /// \return The number of UTF-8 code points in the buffer given by 'str' and 'nbytes'.
        std::size_t length (char const * str, std::size_t nbytes);

        /// Returns the number of UTF-8 code points in the null-terminated buffer at str
        std::size_t length (gsl::czstring str);

        inline std::size_t length (std::nullptr_t) noexcept { return 0; }
        ///@}

        ///@{
        /// Returns a reference to the beginning of the pos'th UTF-8 code-point in a sequence.

        /// Returns a pointer to the beginning of the pos'th UTF-8 codepoint
        /// in the buffer at str
        char const * index (gsl::czstring str, std::size_t pos);

        /// Returns an iterator to the beginning of the pos'th UTF-8 codepoint
        /// in the range given by first and last.
        ///
        /// \param first  The start of the range of elements to examine
        /// \param last  The end of the range of elements to examine
        /// \param pos  The number of code points to move
        /// \returns  An iterator that is 'pos' codepoints after the start of the range or
        ///           'last' if the end of the range was encountered.
        template <typename InputIterator>
        InputIterator index (InputIterator first, InputIterator last, std::size_t pos) {
            auto start_count = std::size_t{0};
            return std::find_if (first, last, [&start_count, pos](char c) {
                return is_utf_char_start (c) ? (start_count++ == pos) : false;
            });
        }

        /// Returns a pointer to the beginning of the pos'th UTF-8 codepoint
        /// in the supplied span.
        template <typename SpanType>
        typename SpanType::element_type * index (SpanType span, std::size_t pos) {
            auto end = span.end ();
            auto it = index (span.begin (), end, pos);
            return it == end ? nullptr : &*it;
        }

        ///@}


        /// Converts codepoint indices start and end to byte offsets in the buffer at str
        ///
        /// \param str    A UTF-8 encoded character string.
        /// \param start  The code-point index of the start of a character range within the string
        /// 'str'.
        /// \param end    The code-point index of the end of a character range within the string
        /// 'str'.
        /// \return A pair containing the the byte offset of the start UTF-8 code-unit and the byte
        /// offset of the end UTF-8 code-unit. Either value may be -1 if they were out-of-range.
        std::pair<std::ptrdiff_t, std::ptrdiff_t> slice (char const * str, std::ptrdiff_t start,
                                                         std::ptrdiff_t end);


        using native_string = std::basic_string<TCHAR>;
        using native_ostringstream = std::basic_ostringstream<TCHAR>;

#if defined(_WIN32)
#if defined(_UNICODE)
        inline std::wstring to_native_string (std::string const & str) {
            return utf::win32::to16 (str);
        }
        inline std::wstring to_native_string (char const * str) { return utf::win32::to16 (str); }
        inline std::string from_native_string (std::wstring const & str) {
            return utf::win32::to8 (str);
        }
        inline std::string from_native_string (wchar_t const * str) {
            return utf::win32::to8 (str);
        }
#else
        // This is Windows in "Multibyte character set" mode.
        inline std::string to_native_string (std::string const & str) {
            return win32::to_mbcs (str);
        }
        inline std::string to_native_string (char const * str) { return win32::to_mbcs (str); }
#endif //_UNICODE
        inline std::string from_native_string (std::string const & str) {
            return win32::mbcs_to8 (str);
        }
        inline std::string from_native_string (char const * str) { return win32::mbcs_to8 (str); }
#else //_WIN32
        inline std::string to_native_string (std::string const & str) { return str; }
        inline std::string to_native_string (char const * str) { return str; }
        inline std::string from_native_string (std::string const & str) { return str; }
        inline std::string from_native_string (char const * str) { return str; }
#endif
    } // namespace utf
} // namespace pstore

#endif // PSTORE_SUPPORT_UTF_HPP
