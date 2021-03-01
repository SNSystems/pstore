//===- include/pstore/http/request.hpp --------------------*- mode: C++ -*-===//
//*                                 _    *
//*  _ __ ___  __ _ _   _  ___  ___| |_  *
//* | '__/ _ \/ _` | | | |/ _ \/ __| __| *
//* | | |  __/ (_| | |_| |  __/\__ \ |_  *
//* |_|  \___|\__, |\__,_|\___||___/\__| *
//*              |_|                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file request.hpp
/// \brief Parses HTTP request strings.

#ifndef PSTORE_HTTP_REQUEST_HPP
#define PSTORE_HTTP_REQUEST_HPP

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include "pstore/adt/error_or.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace http {

        class request_info {
        public:
            request_info (std::string && m, std::string && u, std::string && v)
                    : method_{std::move (m)}
                    , uri_{std::move (u)}
                    , version_{std::move (v)} {}
            request_info (request_info const &) = default;
            request_info (request_info &&) noexcept = default;

            ~request_info () noexcept = default;

            request_info & operator= (request_info const &) = delete;
            request_info & operator= (request_info &&) noexcept = delete;

            std::string const & method () const { return method_; }
            std::string const & version () const { return version_; }
            std::string const & uri () const { return uri_; }

        private:
            std::string method_;
            std::string uri_;
            std::string version_;
        };

        namespace details {

            inline std::error_code out_of_data_error () {
                // TODO: we're returning that we ran out of data. Is there a more suitable
                // error?
                return std::make_error_code (std::errc::not_connected);
            }

            inline decltype (auto) skip_leading_ws (std::string const & s,
                                                    std::string::size_type pos) noexcept {
                auto const length = s.length ();
                while (pos < length && std::isspace (s[pos])) {
                    ++pos;
                }
                return pos;
            }

        } // end namespace details

        // read_request
        // ~~~~~~~~~~~~
        /// \tparam Reader  The buffered_reader<> type from which data is to be read.
        /// \param reader  An instance of Reader: a buffered_reader<> from which data is read,
        /// \param io  The state passed to the reader's refill function.
        /// \returns  Type error_or_n<Reader::state_type, request_info>. Either an error or the
        /// updated reader state value and an instance of request_info containing the HTTP method,
        /// URI and version strings.
        template <typename Reader>
        error_or_n<typename Reader::state_type, request_info>
        read_request (Reader & reader, typename Reader::state_type io) {
            using state_type = typename Reader::state_type;

            auto check_for_eof = [] (state_type io2, maybe<std::string> const & buf) {
                using result_type = error_or_n<state_type, std::string>;
                if (!buf) {
                    return result_type{details::out_of_data_error ()};
                }
                return result_type{in_place, io2, *buf};
            };

            auto extract_request_info = [] (state_type io3, std::string const & s) {
                using result_type = error_or_n<state_type, request_info>;

                std::istringstream is{s};
                std::string method;
                std::string uri;
                std::string version;
                is >> method >> uri >> version;
                if (method.length () == 0 || uri.length () == 0 || version.length () == 0) {
                    return result_type{details::out_of_data_error ()};
                }
                return result_type{
                    in_place, io3,
                    request_info{std::move (method), std::move (uri), std::move (version)}};
            };

            return (reader.gets (io) >>= check_for_eof) >>= extract_request_info;
        }

        // read_headers
        // ~~~~~~~~~~~~
        /// \tparam Reader  The buffered_reader<> type from which data is to be read.
        /// \tparam HandleFn A function of the form `std::function<IO(IO, std::string const &,
        /// std::string const &)>`.
        /// \tparam IO The type of the state object passed to handler().
        ///
        /// \param reader  An instance of Reader: a buffered_reader<> from which data is read.
        /// \param reader_state  The state passed to the reader's refill function.
        /// \param handler  A function called for each HTTP header. It is passed a state value, the
        /// key (lower-cased to ensure case insensitivity) and the associated value.
        /// \param handler_state  The state passed to handler.
        template <typename Reader, typename HandleFn, typename IO>
        error_or_n<typename Reader::state_type, IO>
        read_headers (Reader & reader, typename Reader::state_type reader_state, HandleFn handler,
                      IO handler_state) {
            using return_type = error_or_n<typename Reader::state_type, IO>;
            return reader.gets (reader_state) >>=
                   [&reader, &handler,
                    &handler_state] (typename Reader::state_type state2,
                                     maybe<std::string> const & ms) -> return_type {
                if (!ms || ms->length () == 0) {
                    return return_type{in_place, state2, handler_state};
                }

                std::string key;
                std::string value;

                auto pos = ms->find (':');
                if (pos == std::string::npos) {
                    value = *ms;
                } else {
                    key = ms->substr (0, pos);
                    // HTTP header names are case-insensitive so convert to lower-case here.
                    std::transform (std::begin (key), std::end (key), std::begin (key),
                                    [] (unsigned char const c) {
                                        return static_cast<char> (std::tolower (c));
                                    });
                    ++pos; // Skip the colon.
                    // Skip optional whitespace before the value string.
                    value = ms->substr (details::skip_leading_ws (*ms, pos));
                }

                return read_headers (reader, state2, handler, handler (handler_state, key, value));
            };
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_REQUEST_HPP
