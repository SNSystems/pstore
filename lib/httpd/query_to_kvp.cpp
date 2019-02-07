//*                                _          _                  *
//*   __ _ _   _  ___ _ __ _   _  | |_ ___   | | ____   ___ __   *
//*  / _` | | | |/ _ \ '__| | | | | __/ _ \  | |/ /\ \ / / '_ \  *
//* | (_| | |_| |  __/ |  | |_| | | || (_) | |   <  \ V /| |_) | *
//*  \__, |\__,_|\___|_|   \__, |  \__\___/  |_|\_\  \_/ | .__/  *
//*     |_|                |___/                         |_|     *
//===- lib/httpd/query_to_kvp.cpp -----------------------------------------===//
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
#include "pstore/httpd/query_to_kvp.hpp"

#include <cassert>
#include <cctype>
#include <cstring>
#include <ostream>
#include <iterator>
#include <tuple>
//#include <boost/iterator_adaptors.hpp>
//#include <boost/foreach.hpp>

//#include "const_ptr_iterator.h"

namespace {

    template <typename CharType>
    int hex_digit (CharType c) noexcept {
        assert (std::isxdigit (c));
        if (std::isdigit (c)) {
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

        return std::make_tuple (value, it);
    }



    template <typename Iterator,
              typename ResultContainer = std::unordered_map<std::string, std::string>>
    ResultContainer parse_query (Iterator begin, Iterator end) {
        ResultContainer result;

        std::string key;
        std::string data;

        enum { key_mode, value_mode } state = key_mode;

        for (Iterator it = begin; it != end;) {
            bool do_append = true;
            auto c = *(it++);
            switch (c) {
                // Within a query component, the characters ";", "/", "?", ":", "@",
                // "&", "=", "+", ",", and "$" are reserved. (c.f. rfc2396)
            case '/':
            case '?':
            case ':':
            case '@':
            case ',':
            case '$': break;

            case '+': c = ' '; break;

            case '%': std::tie (c, it) = value_from_hex (it, end); break;

            case '=':
                if (state == key_mode) {
                    state = value_mode;
                    key = data;
                    data.clear ();
                }
                do_append = false;
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
                        result[key] = data;
                    }
                    state = key_mode;
                    data.clear ();
                }
                do_append = false;
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
            result[key] = data;
        }

        return result;
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        query_kvp query_to_kvp (gsl::czstring in) {
            return parse_query (in, in + std::strlen (in));
        }

        query_kvp query_to_kvp (std::string const & in) {
            return parse_query (in.begin (), in.end ());
        }

    } // end namespace httpd
} // end namespace pstore
