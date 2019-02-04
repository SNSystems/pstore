//*        _    __  *
//*  _   _| |_ / _| *
//* | | | | __| |_  *
//* | |_| | |_|  _| *
//*  \__,_|\__|_|   *
//*                 *
//===- include/pstore/json/utf.hpp ----------------------------------------===//
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
#ifndef PSTORE_JSON_UTF_HPP
#define PSTORE_JSON_UTF_HPP

#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <string>

#include <tuple>
#include <type_traits>
#include <vector>

#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace json {

        using utf8_string = std::basic_string<std::uint8_t>;
        using utf16_string = std::basic_string<char16_t>;

        std::ostream & operator<< (std::ostream & os, utf8_string const & s);

        class utf8_decoder {
        public:
            std::tuple<char32_t, bool> get (std::uint8_t c) noexcept;
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


        extern char32_t const replacement_char_code_point;

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

    } // namespace json
} // namespace pstore

#endif // PSTORE_JSON_UTF_HPP
