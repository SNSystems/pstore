//===- include/pstore/http/serve_dynamic_content.hpp ------*- mode: C++ -*-===//
//*                                _                             _       *
//*  ___  ___ _ ____   _____    __| |_   _ _ __   __ _ _ __ ___ (_) ___  *
//* / __|/ _ \ '__\ \ / / _ \  / _` | | | | '_ \ / _` | '_ ` _ \| |/ __| *
//* \__ \  __/ |   \ V /  __/ | (_| | |_| | | | | (_| | | | | | | | (__  *
//* |___/\___|_|    \_/ \___|  \__,_|\__, |_| |_|\__,_|_| |_| |_|_|\___| *
//*                                  |___/                               *
//*                  _             _    *
//*   ___ ___  _ __ | |_ ___ _ __ | |_  *
//*  / __/ _ \| '_ \| __/ _ \ '_ \| __| *
//* | (_| (_) | | | | ||  __/ | | | |_  *
//*  \___\___/|_| |_|\__\___|_| |_|\__| *
//*                                     *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_SERVE_DYNAMIC_CONTENT_HPP
#define PSTORE_HTTP_SERVE_DYNAMIC_CONTENT_HPP

#include <cassert>
#include <functional>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "pstore/adt/error_or.hpp"
#include "pstore/core/file_header.hpp"
#include "pstore/http/error.hpp"
#include "pstore/http/http_date.hpp"
#include "pstore/http/net_txrx.hpp"
#include "pstore/http/query_to_kvp.hpp"
#include "pstore/http/quit.hpp"
#include "pstore/http/send.hpp"
#include "pstore/json/utility.hpp"
#include "pstore/support/array_elements.hpp"

namespace pstore {
    namespace http {

        constexpr char dynamic_path[] = "/cmd/";

        using query_container = std::unordered_map<std::string, std::string>;

        template <typename Sender, typename IO>
        pstore::error_or<IO> handle_version (Sender sender, IO io, query_container const &) {
            auto version_string = [] () {
                std::ostringstream os;
                os << R"({ "version": ")" << header::major_version << '.' << header::minor_version
                   << "\" }";
                auto const & v = os.str ();
                PSTORE_ASSERT (json::is_valid (v));
                return v;
            };
            static std::string const version = version_string ();
            static auto const modified = std::chrono::system_clock::now ();

            std::ostringstream os;
            os << "HTTP/1.1 200 OK" << crlf                                         //
               << "Connection: close" << crlf                                       //
               << "Content-length: " << version.length () << crlf                   //
               << "Content-type: application/json" << crlf                          //
               << "Date: " << http_date (std::chrono::system_clock::now ()) << crlf //
               << "Last-Modified: " << http_date (modified) << crlf                 //
               << "Server: " << server_name << crlf                                 //
               << crlf // End of headers
               << version;
            return pstore::http::send (sender, io, os.str ());
        }


        namespace details {

            // TODO: there's a similar function in command_line.cpp
            // Returns true if the string \p s starts with the given prefix.
            inline bool starts_with (std::string const & s, std::string const & prefix) {
                std::string::size_type pos = 0;
                while (pos < s.length () && pos < prefix.length () && s[pos] == prefix[pos]) {
                    ++pos;
                }
                return pos == prefix.length ();
            }

            template <typename T>
            auto clamp_to_signed_max (T v) noexcept -> typename std::make_signed<T>::type {
                using st = typename std::make_signed<T>::type;
                constexpr auto maxs = std::numeric_limits<st>::max ();
                PSTORE_STATIC_ASSERT (maxs >= 0);
                return static_cast<st> (std::min (static_cast<T> (maxs), v));
            }


            template <typename Sender, typename IO>
            struct commands_helper {
                using return_type = error_or<IO>;
                using function_type =
                    std::function<return_type (Sender, IO, query_container const &)>;

                using container = std::array<std::pair<std::string, function_type>, 1>;
            };

            template <typename Sender, typename IO>
            typename commands_helper<Sender, IO>::container const & get_commands () {
                static typename commands_helper<Sender, IO>::container const commands = {
                    {{"version", handle_version<Sender, IO>}},
                };
                return commands;
            }

        } // end namespace details


        template <typename Sender, typename IO>
        error_or<IO> serve_dynamic_content (Sender sender, IO io, std::string uri) {

            // Remove the common path prefix from the URI.
            PSTORE_ASSERT (details::starts_with (uri, dynamic_path));
            uri.erase (std::begin (uri),
                       std::begin (uri) + pstore::array_elements (dynamic_path) - 1U);

            // Extract the command name and any query argument, if we have them.
            auto const pos = uri.find ('?');
            auto const command = uri.substr (0, pos);
            auto it = pos != std::string::npos
                          ? std::begin (uri) + details::clamp_to_signed_max (pos + 1)
                          : std::end (uri);
            query_container arguments;
            if (it != std::end (uri)) {
                it = query_to_kvp (it, std::end (uri),
                                   pstore::http::make_insert_iterator (arguments));
            }

            // Do we know how to handle this command?
            auto const & commands = details::get_commands<Sender, IO> ();
            using value_type = typename details::commands_helper<Sender, IO>::container::value_type;
            auto const compare = [] (value_type const & a, value_type const & b) {
                return std::get<0> (a) < std::get<0> (b);
            };
            PSTORE_ASSERT (std::is_sorted (std::begin (commands), std::end (commands), compare));

            auto const lb = std::lower_bound (
                std::begin (commands), std::end (commands),
                value_type{command, [] (Sender, IO io2,
                                        query_container const &) { return error_or<IO>{io2}; }},
                compare);
            if (lb == std::end (commands) || std::get<0> (*lb) != command) {
                return error_or<IO>{error_code::bad_request};
            }

            // Yep, this is a command we understand. Call it.
            return std::get<1> (*lb) (sender, io, arguments);
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_SERVE_DYNAMIC_CONTENT_HPP
