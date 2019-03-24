//*                                 _    *
//*  _ __ ___  __ _ _   _  ___  ___| |_  *
//* | '__/ _ \/ _` | | | |/ _ \/ __| __| *
//* | | |  __/ (_| | |_| |  __/\__ \ |_  *
//* |_|  \___|\__, |\__,_|\___||___/\__| *
//*              |_|                     *
//===- include/pstore/httpd/request.hpp -----------------------------------===//
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

#ifndef PSTORE_HTTPD_REQUEST_HPP
#define PSTORE_HTTPD_REQUEST_HPP

#include <cassert>
#include <cctype>
#include <cerrno>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include "pstore/support/error_or.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace httpd {

        class request_info {
        public:
            request_info (std::string && m, std::string && u, std::string && v)
                    : method_{std::move (m)}
                    , uri_{std::move (u)}
                    , version_{std::move (v)} {}
            request_info (request_info const &) = default;
            request_info (request_info &&) = default;
            ~request_info () = default;

            request_info & operator= (request_info const &) = delete;
            request_info & operator= (request_info &&) = delete;

            std::string const & method () const { return method_; }
            std::string const & version () const { return version_; }
            std::string const & uri () const { return uri_; }

        private:
            std::string method_;
            std::string uri_;
            std::string version_;
        };

        namespace details {
            std::error_code out_of_data_error () {
                // TODO: we're returning that we ran out of data. Is there a more suitable
                // error?
                return std::make_error_code (std::errc (ENOTCONN));
            }
        } // end namespace details

        // read_request
        // ~~~~~~~~~~~~
        template <typename ReaderType>
        error_or<std::pair<typename ReaderType::state_type, request_info>>
        read_request (ReaderType & reader, typename ReaderType::state_type io) {
            using state_type = typename ReaderType::state_type;

            auto check_for_eof = [](std::pair<state_type, maybe<std::string>> const & p) {
                using result_type = error_or<std::pair<state_type, std::string>>;
                auto const & buf = std::get<1> (p);
                if (!buf) {
                    return result_type{details::out_of_data_error ()};
                }
                return result_type{pstore::in_place, std::get<0> (p), *buf};
            };

            auto extract_request_info = [](std::pair<state_type, std::string> const & s) {
                using result_type = error_or<std::pair<state_type, request_info>>;

                std::istringstream str{std::get<1> (s)};
                std::string method;
                std::string uri;
                std::string version;
                str >> method >> uri >> version;
                if (method.length () == 0 || uri.length () == 0 || version.length () == 0) {
                    return result_type{details::out_of_data_error ()};
                }
                return result_type{
                    pstore::in_place, std::get<0> (s),
                    request_info{std::move (method), std::move (uri), std::move (version)}};
            };

            return (reader.gets (io) >>= check_for_eof) >>= extract_request_info;
        }

        // read_headers
        // ~~~~~~~~~~~~
        /// \tparam Reader  The buffered_reader<> type from which data is to be read.
        /// \tparam HandleFn A function of the form `std::function<IO(IO, std::string const &,
        /// std::string const &)>`. \tparam IO The type of the state object passed to handler().
        /// \param reader  An instance of Reader: a buffered_reader<> from which data is read.
        /// \param reader_state  The state passed to the reader's refill function.
        /// \param handler  A function called for each HTTP header. It is passed a state value, the
        /// key (lower-cased to ensure case insensitivity) and the associate value. \param
        /// handler_state  The state passed to handler.
        template <typename Reader, typename HandleFn, typename IO>
        error_or<std::pair<typename Reader::state_type, IO>>
        read_headers (Reader & reader, typename Reader::state_type reader_state, HandleFn handler,
                      IO handler_state) {
            return reader.gets (reader_state) >>=
                   [&reader, &handler, &handler_state](
                       std::pair<typename Reader::state_type, maybe<std::string>> const & p) {
                       auto const & ms = std::get<1> (p);
                       if (!ms || ms->length () == 0) {
                           return error_or<std::pair<typename Reader::state_type, IO>>{
                               pstore::in_place, std::get<0> (p), handler_state};
                       }

                       std::string key;
                       std::string value;
                       auto pos = ms->find (':');
                       if (pos == std::string::npos) {
                           value = *ms;
                       } else {
                           key = ms->substr (0, pos);
                           // HTTP header names are case-insensitive so convert to lower-case here.
                           std::transform (key.begin (), key.end (), key.begin (), [](char c) {
                               return static_cast<char> (std::tolower (c));
                           });

                           ++pos; // skip the colon
                           // skip optional whitespace before the value string.
                           while (pos < ms->length () && std::isspace ((*ms)[pos])) {
                               ++pos;
                           }
                           value = ms->substr (pos);
                       }

                       return read_headers (reader, std::get<0> (p), handler,
                                            handler (handler_state, key, value));
                   };
        }

    } // namespace httpd
} // namespace pstore

#endif // PSTORE_HTTPD_REQUEST_HPP
