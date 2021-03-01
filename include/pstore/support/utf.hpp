//===- include/pstore/support/utf.hpp ---------------------*- mode: C++ -*-===//
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
/// \file support/utf.hpp
/// \brief Functionality for processing UTF-8 strings. On Windows, provides an
/// additional set of functions to convert UTF-8 strings to and from UTF-16.

#ifndef PSTORE_SUPPORT_UTF_HPP
#define PSTORE_SUPPORT_UTF_HPP

#include <algorithm>
#include <cstddef>
#include <iosfwd>

#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

#if defined(_WIN32)

#    include <tchar.h>

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
                PSTORE_ASSERT (str != nullptr);
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
                PSTORE_ASSERT (str != nullptr);
                return mbcs_to8 (str, std::strlen (str));
            }

            /// \param str  A string of "multi-byte" characters to be converted to UTF-8.
            inline std::string mbcs_to8 (std::string const & str) {
                return mbcs_to8 (str.data (), str.length ());
            }
            ///@}
        } // end namespace win32
    }     // end namespace utf
} // end namespace pstore

#else //_WIN32

using TCHAR = char;

#endif // !defined(_WIN32)


namespace pstore {
    namespace utf {

        using utf8_string = std::basic_string<std::uint8_t>;
        using utf16_string = std::basic_string<char16_t>;

        auto operator<< (std::ostream & os, utf8_string const & s) -> std::ostream &;

        class utf8_decoder {
        public:
            auto get (std::uint8_t byte) noexcept -> maybe<char32_t>;
            auto is_well_formed () const noexcept -> bool { return well_formed_; }

        private:
            enum state { accept, reject };

            static auto decode (gsl::not_null<std::uint8_t *> state,
                                gsl::not_null<char32_t *> codep, std::uint32_t byte) noexcept
                -> std::uint8_t;

            static std::uint8_t const utf8d_[];
            char32_t codepoint_ = 0;
            std::uint8_t state_ = accept;
            bool well_formed_ = true;
        };


        constexpr char32_t replacement_char_code_point = 0xFFFD;

        template <typename CharType = char, typename OutputIt>
        auto code_point_to_utf8 (char32_t c, OutputIt out) -> OutputIt;


        template <typename CharType = char, typename OutputIt>
        auto replacement_char (OutputIt out) -> OutputIt {
            return code_point_to_utf8<CharType> (replacement_char_code_point, out);
        }

        template <typename CharType, typename OutputIt>
        auto code_point_to_utf8 (char32_t const c, OutputIt out) -> OutputIt {
            if (c < 0x80) {
                *(out++) = static_cast<CharType> (c);
            } else if (c < 0x800) {
                *(out++) = static_cast<CharType> (c / 64U + 0xC0U);
                *(out++) = static_cast<CharType> (c % 64U + 0x80U);
            } else if ((c >= 0xD800 && c < 0xE000) || c >= 0x110000) {
                out = replacement_char<CharType> (out);
            } else if (c < 0x10000) {
                *(out++) = static_cast<CharType> ((c / 0x1000U) | 0xE0U);
                *(out++) = static_cast<CharType> ((c / 64U % 64U) | 0x80U);
                *(out++) = static_cast<CharType> ((c % 64U) | 0x80U);
            } else {
                PSTORE_ASSERT (c < 0x110000);
                *(out++) = static_cast<CharType> ((c / 0x40000U) | 0xF0U);
                *(out++) = static_cast<CharType> ((c / 0x1000U % 64U) | 0x80U);
                *(out++) = static_cast<CharType> ((c / 64U % 64U) | 0x80U);
                *(out++) = static_cast<CharType> ((c % 64U) | 0x80U);
            }
            return out;
        }

        template <typename ResultType>
        auto code_point_to_utf8 (char32_t c) -> ResultType {
            ResultType result;
            code_point_to_utf8<typename ResultType::value_type> (c, std::back_inserter (result));
            return result;
        }

        constexpr auto nop_swapper (std::uint16_t const v) noexcept -> std::uint16_t { return v; }
        constexpr auto byte_swapper (std::uint16_t const v) noexcept -> std::uint16_t {
            return static_cast<std::uint16_t> (((v & 0x00FFU) << 8U) | ((v & 0xFF00U) >> 8U));
        }

        constexpr auto is_utf16_high_surrogate (std::uint16_t const code_unit) noexcept -> bool {
            return code_unit >= 0xD800 && code_unit <= 0xDBFF;
        }

        // is_utf16_low_surrogate
        // ~~~~~~~~~~~~~~~~~~~~~~
        constexpr auto is_utf16_low_surrogate (std::uint16_t const code_unit) noexcept -> bool {
            return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
        }

        // utf16_to_code_point
        // ~~~~~~~~~~~~~~~~~~~
        template <typename InputIterator, typename SwapperFunction>
        auto utf16_to_code_point (InputIterator first, InputIterator last, SwapperFunction swapper)
            -> std::pair<InputIterator, char32_t> {

            using value_type = typename std::remove_cv<
                typename std::iterator_traits<InputIterator>::value_type>::type;
            static_assert (std::is_same<value_type, char16_t>::value,
                           "iterator must produce char16_t");

            PSTORE_ASSERT (first != last);
            char32_t code_point = 0;
            char16_t const code_unit = swapper (*(first++));
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
        auto utf16_to_code_points (InputIt first, InputIt last, OutputIt out, Swapper swapper)
            -> OutputIt {
            while (first != last) {
                char32_t code_point;
                std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
                *(out++) = code_point;
            }
            return out;
        }

        template <typename ResultType, typename InputIt, typename Swapper>
        auto utf16_to_code_points (InputIt first, InputIt last, Swapper swapper) -> ResultType {
            ResultType result;
            utf16_to_code_points (first, last, std::back_inserter (result), swapper);
            return result;
        }

        template <typename ResultType, typename InputType, typename Swapper>
        auto utf16_to_code_points (InputType const & src, Swapper swapper) -> ResultType {
            return utf16_to_code_points<ResultType> (std::begin (src), std::end (src), swapper);
        }

        // utf16_to_code_point
        // ~~~~~~~~~~~~~~~~~~~
        template <typename InputType, typename Swapper>
        auto utf16_to_code_point (InputType const & src, Swapper swapper) -> char32_t {
            auto end = std::end (src);
            char32_t cp;
            std::tie (end, cp) = utf16_to_code_point (std::begin (src), end, swapper);
            PSTORE_ASSERT (end == std::end (src));
            return cp;
        }

        // utf16_to_utf8
        // ~~~~~~~~~~~~~
        template <typename InputIt, typename OutputIt, typename Swapper>
        auto utf16_to_utf8 (InputIt first, InputIt last, OutputIt out, Swapper swapper)
            -> OutputIt {
            while (first != last) {
                char32_t code_point;
                std::tie (first, code_point) = utf16_to_code_point (first, last, swapper);
                out = code_unit_to_utf8 (code_point, out);
            }
            return out;
        }

        template <typename ResultType, typename InputIt, typename Swapper>
        auto utf16_to_utf8 (InputIt first, InputIt last, Swapper swapper) -> ResultType {
            ResultType result;
            utf16_to_utf8 (first, last, std::back_inserter (result), swapper);
            return result;
        }

        template <typename ResultType, typename InputType, typename Swapper>
        auto utf16_to_utf8 (InputType const & src, Swapper swapper) -> ResultType {
            return utf16_to_utf8<ResultType> (std::begin (src), std::end (src), swapper);
        }

    } // end namespace utf
} // end namespace pstore



namespace pstore {
    namespace utf {

        /// If the top two bits are 0b10, then this is a UTF-8 continuation byte
        /// and is skipped; other patterns in these top two bits represent the
        /// start of a character.
        template <typename CharType>
        constexpr auto is_utf_char_start (CharType c) noexcept -> bool {
            using uchar_type = typename std::make_unsigned<CharType>::type;
            return (static_cast<uchar_type> (c) & 0xC0U) != 0x80U;
        }

        ///@{
        /// Returns the number of UTF-8 code-points in a sequence.

        template <typename Iterator>
        auto length (Iterator first, Iterator last) -> std::size_t {
            auto const result =
                std::count_if (first, last, [] (char const c) { return is_utf_char_start (c); });
            PSTORE_ASSERT (result >= 0);
            using utype = typename std::make_unsigned<decltype (result)>::type;
            static_assert (std::numeric_limits<utype>::max () <=
                               std::numeric_limits<std::size_t>::max (),
                           "std::size_t cannot hold result of count_if");
            return static_cast<std::size_t> (result);
        }

        template <typename SpanType>
        auto length (SpanType span) -> std::size_t {
            return length (span.begin (), span.end ());
        }

        /// Returns the number of UTF-8 code points in the buffer given by a start address and
        /// length.
        ///
        /// \param str  The buffer start address.
        /// \param nbytes The number of bytes in the buffer.
        /// \return The number of UTF-8 code points in the buffer given by 'str' and 'nbytes'.
        auto length (char const * str, std::size_t nbytes) -> std::size_t;

        /// Returns the number of UTF-8 code points in the null-terminated buffer at str
        auto length (gsl::czstring str) -> std::size_t;
        auto length (std::string const & str) -> std::size_t;

        inline auto length (std::nullptr_t) noexcept -> std::size_t { return 0; }
        ///@}

        ///@{
        /// Returns a reference to the beginning of the pos'th UTF-8 code-point in a sequence.

        /// Returns a pointer to the beginning of the pos'th UTF-8 codepoint
        /// in the buffer at \p str or nullptr if either \p str is nullptr or if \p index was too
        /// large.
        auto index (gsl::czstring str, std::size_t pos) -> gsl::czstring;

        /// Returns an iterator to the beginning of the pos'th UTF-8 codepoint
        /// in the range given by first and last.
        ///
        /// \param first  The start of the range of elements to examine
        /// \param last  The end of the range of elements to examine
        /// \param pos  The number of code points to move
        /// \returns  An iterator that is 'pos' codepoints after the start of the range or
        ///           'last' if the end of the range was encountered.
        template <typename InputIterator>
        auto index (InputIterator const first, InputIterator const last, std::size_t const pos)
            -> InputIterator {
            auto start_count = std::size_t{0};
            return std::find_if (first, last, [&start_count, pos] (char const c) {
                return is_utf_char_start (c) ? (start_count++ == pos) : false;
            });
        }

        /// Returns a pointer to the beginning of the pos'th UTF-8 codepoint
        /// in the supplied span.
        ///
        /// \param span  A span of memory containing a sequence of UTF-8 codepoints.
        /// \param pos  The number of code points to move
        /// \returns  A pointer that is 'pos' codepoints after the start of \p span or
        ///           nullptr if the end of the range was encountered.
        template <typename SpanType>
        auto index (SpanType const span, std::size_t const pos) ->
            typename SpanType::element_type * {
            auto const end = span.end ();
            auto const it = index (span.begin (), end, pos);
            return it == end ? nullptr : &*it;
        }

        ///@}


        /// Converts codepoint indices start and end to byte offsets in the buffer at \p str.
        ///
        /// \param str    A UTF-8 encoded character string.
        /// \param start  The code-point index of the start of a character range within the string
        /// 'str'.
        /// \param end    The code-point index of the end of a character range within the string
        /// 'str'.
        /// \return A pair containing the the byte offset of the start UTF-8 code-unit and the byte
        /// offset of the end UTF-8 code-unit. Either value may be -1 if they were out-of-range.
        auto slice (gsl::czstring str, std::ptrdiff_t start, std::ptrdiff_t end)
            -> std::pair<std::ptrdiff_t, std::ptrdiff_t>;


        using native_string = std::basic_string<TCHAR>;
        using native_ostringstream = std::basic_ostringstream<TCHAR>;

#if defined(_WIN32)
#    if defined(_UNICODE)
        inline auto to_native_string (std::string const & str) -> std::wstring {
            return utf::win32::to16 (str);
        }
        inline auto to_native_string (gsl::czstring const str) -> std::wstring {
            return utf::win32::to16 (str);
        }
        inline auto from_native_string (std::wstring const & str) -> std::string {
            return utf::win32::to8 (str);
        }
        inline auto from_native_string (gsl::cwzstring const str) -> std::string {
            return utf::win32::to8 (str);
        }
#    else
        // This is Windows in "Multibyte character set" mode.
        inline auto to_native_string (std::string const & str) -> std::string {
            return win32::to_mbcs (str);
        }
        inline auto to_native_string (gsl::czstring const str) -> std::string {
            return win32::to_mbcs (str);
        }
#    endif //_UNICODE
        inline auto from_native_string (std::string const & str) -> std::string {
            return win32::mbcs_to8 (str);
        }
        inline auto from_native_string (gsl::czstring const str) -> std::string {
            return win32::mbcs_to8 (str);
        }
#else //_WIN32
        constexpr auto to_native_string (std::string const & str) noexcept -> std::string const & {
            return str;
        }
        constexpr auto from_native_string (std::string const & str) noexcept
            -> std::string const & {
            return str;
        }
#endif
    } // end namespace utf
} // end namespace pstore

#endif // PSTORE_SUPPORT_UTF_HPP
