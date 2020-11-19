//*                                _          _                  *
//*   __ _ _   _  ___ _ __ _   _  | |_ ___   | | ____   ___ __   *
//*  / _` | | | |/ _ \ '__| | | | | __/ _ \  | |/ /\ \ / / '_ \  *
//* | (_| | |_| |  __/ |  | |_| | | || (_) | |   <  \ V /| |_) | *
//*  \__, |\__,_|\___|_|   \__, |  \__\___/  |_|\_\  \_/ | .__/  *
//*     |_|                |___/                         |_|     *
//===- include/pstore/http/query_to_kvp.hpp -------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
/// \file query_to_kvp.hpp
/// \brief Functions for converting key/value pairs to and from the query component of a URI.
#ifndef PSTORE_HTTP_QUERY_TO_KVP_HPP
#define PSTORE_HTTP_QUERY_TO_KVP_HPP

#include <algorithm>
#include <cctype>
#include <cstring>
#include <tuple>

#include "pstore/support/gsl.hpp"
#include "pstore/support/portab.hpp"

namespace pstore {
    namespace httpd {

        namespace details {

            template <typename InputIt, typename OutputIt>
            OutputIt escape (InputIt first, InputIt last, OutputIt out) {
                std::for_each (first, last, [&out] (char c) {
                    auto const is_unreserved_char = [] (char const c2) {
                        return (c2 >= 'A' && c2 <= 'Z') || (c2 >= 'a' && c2 <= 'z') ||
                               (c2 >= '0' && c2 <= '9') || c2 == '-' || c2 == '.' || c2 == '_' ||
                               c2 == '~';
                    };
                    auto const nibble_to_hex_char = [] (unsigned const n) {
                        auto const c2 = n & 0x0FU;
                        return static_cast<char> (c2 < 10 ? c2 + '0' : c2 - 10 + 'A');
                    };

                    if (is_unreserved_char (c)) {
                        *(out++) = c;
                    } else {
                        *(out++) = '%';
                        *(out++) = nibble_to_hex_char (static_cast<unsigned> (c) >> 4U);
                        *(out++) = nibble_to_hex_char (static_cast<unsigned> (c));
                    }
                });
                return out;
            }

        } // end namespace details

        /// Converts a container of a two-tuple or pair of std::string (such as
        /// std::map<std::string, std::string> or std::unordered_map<std::string, std::string> to a
        /// URI query string.
        ///
        /// \param first  The first of the range of elements to convert.
        /// \param last  End beyond the end of the range of elements to convert.
        /// \param out  The beginning of the destination range.
        template <typename InputIt, typename OutputIt>
        OutputIt kvp_to_query (InputIt first, InputIt last, OutputIt out) {
            auto is_first = true;
            auto const f = [&is_first, &out] (
                               typename std::iterator_traits<InputIt>::value_type const & value) {
                if (is_first) {
                    is_first = false;
                } else {
                    *(out++) = '&';
                }

                out =
                    details::escape (std::get<0> (value).begin (), std::get<0> (value).end (), out);
                *(out++) = '=';
                out =
                    details::escape (std::get<1> (value).begin (), std::get<1> (value).end (), out);
            };
            std::for_each (first, last, f);
            return out;
        }

        /// An output iterator which calls insert() on a container when a vaue is assigned to it.
        template <typename Container>
        class insert_iterator {
        public:
            using container_type = Container;

            using iterator_category = std::output_iterator_tag;
            using value_type = void;
            using difference_type = void;
            using pointer = void;
            using reference = void;

            explicit insert_iterator (Container & container) noexcept
                    : container_{&container} {}
            insert_iterator (insert_iterator const &) noexcept = default;
            insert_iterator (insert_iterator &&) noexcept = default;
            ~insert_iterator () noexcept = default;

            insert_iterator & operator= (insert_iterator const &) noexcept = default;
            insert_iterator & operator= (insert_iterator &&) noexcept = default;
            insert_iterator & operator= (typename container_type::value_type const & value) {
                container_->insert (value);
                return *this;
            }

            insert_iterator & operator* () noexcept { return *this; }
            insert_iterator & operator++ () noexcept { return *this; }
            insert_iterator const operator++ (int) noexcept { return *this; }

        private:
            Container * PSTORE_NONNULL container_;
        };

        template <class Container>
        insert_iterator<Container> make_insert_iterator (Container & c) {
            return insert_iterator<Container> (c);
        }


        namespace details {

            template <typename CharType>
            int hex_digit (CharType c) noexcept {
                assert (std::isxdigit (c));
                if (c >= '0' && c <= '9') {
                    return c - '0';
                }
                if (c >= 'a' && c <= 'f') {
                    return c - 'a' + 10;
                }
                if (c >= 'A' && c <= 'F') {
                    return c - 'A' + 10;
                }
                assert (false);
                return 0;
            }

            template <typename Iterator>
            std::tuple<typename std::iterator_traits<Iterator>::value_type, Iterator>
            value_from_hex (Iterator it, Iterator end) {
                using value_type = typename std::iterator_traits<Iterator>::value_type;
                auto value = value_type{0};
                for (auto remaining = 2; it != end && remaining > 0; --remaining, ++it) {
                    value_type const hex_char = *it;
                    if (!std::isxdigit (hex_char)) {
                        break;
                    }
                    value = static_cast<value_type> (value * 16 + hex_digit (hex_char));
                }

                --it;
                return std::make_tuple (value, it);
            }

        } // end namespace details

        template <typename InputIt, typename OutputIt>
        InputIt query_to_kvp (InputIt begin, InputIt end, OutputIt out) {
            std::string key;
            std::string data;

            enum { key_mode, value_mode } state = key_mode;
            bool done = false;
            auto it = begin;
            for (; it != end && !done; ++it) {
                bool do_append = true;
                auto c = *it;
                switch (c) {
                case '#':
                    do_append = false;
                    done = true;
                    break;

                    // Within a query component, the characters ";", "/", "?", ":", "@",
                    // "&", "=", "+", ",", and "$" are reserved. (c.f. rfc2396)
                case '/':
                case '?':
                case ':':
                case '@':
                case ',':
                case '$': break;

                case '+': c = ' '; break;

                case '%': {
                    // Skip the percent character.
                    ++it;
                    if (it == end) {
                        do_append = false;
                        --it;
                    } else {
                        std::tie (c, it) = details::value_from_hex (it, end);
                    }
                } break;

                case '=':
                    if (state == key_mode) {
                        state = value_mode;
                        key = data;
                        data.clear ();
                        do_append = false;
                    }
                    break;

                    // From
                    // <http://www.w3.org/TR/1999/REC-html401-19991224/appendix/notes.html#h-B.2.2>:
                    // "We recommend that HTTP server implementors, and in particular, CGI
                    // implementors support the use of ";" in place of "&" to save authors
                    // the trouble of escaping "&" characters in this manner.
                case ';':
                case '&':
                    if (state == value_mode) {
                        if (key.length () > 0) {
                            *out = std::make_pair (key, data);
                            ++out;
                        }
                        state = key_mode;
                        data.clear ();
                    }
                    do_append = false;
                    break;

                default:
                    // just append the character.
                    do_append = true;
                    break;
                }

                if (do_append) {
                    data.push_back (c);
                }
            }

            // We ran out of input data to process. Before we're done, we need to deal
            // with the final chunk of text that was gathered.
            if (state == key_mode) {
                key = data;
                data.clear ();
            }

            if (key.length () > 0) {
                *out = std::make_pair (key, data);
                ++out;
            }
            return it;
        }



        template <typename OutputIt>
        char const * PSTORE_NONNULL query_to_kvp (gsl::czstring PSTORE_NONNULL in, OutputIt out) {
            return query_to_kvp (in, in + std::strlen (in), out);
        }

        template <typename OutputIt>
        std::string::const_iterator query_to_kvp (std::string const & in, OutputIt out) {
            return query_to_kvp (in.cbegin (), in.cend (), out);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTP_QUERY_TO_KVP_HPP
