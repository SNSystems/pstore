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
//===- include/pstore/httpd/serve_dynamic_content.hpp ---------------------===//
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
#ifndef PSTORE_HTTPD_SERVE_DYNAMIC_CONTENT_HPP
#define PSTORE_HTTPD_SERVE_DYNAMIC_CONTENT_HPP

#include <cassert>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "pstore/httpd/error.hpp"
#include "pstore/httpd/net_txrx.hpp"
#include "pstore/httpd/query_to_kvp.hpp"
#include "pstore/httpd/quit.hpp"
#include "pstore/httpd/send.hpp"
#include "pstore/support/array_elements.hpp"
#include "pstore/support/error_or.hpp"

namespace pstore {
    namespace httpd {

        constexpr char dynamic_path[] = "/cmd/";

        struct server_state {
            bool done = false;
        };

        template <typename IO>
        using command_return_type = pstore::error_or<std::pair<IO, server_state>>;

        using query_container = std::unordered_map<std::string, std::string>;

        template <typename Sender, typename IO>
        command_return_type<IO> handle_quit (Sender sender, IO io, server_state state,
                                             query_container const & query) {
            auto fix_return_type = [&state](IO io2) {
                return command_return_type<IO>{in_place, io2, state};
            };

            auto const pos = query.find ("magic");
            if (pos == std::end (query) || pos->second != pstore::httpd::get_quit_magic ()) {
                return command_return_type<IO>{error_code::bad_request};
            }

            state.done = true;

            static constexpr char quit_message[] =
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>pstore-httpd Exiting</title></head>\n"
                "<body><h1>pstore-httpd Exiting</h1></body>\n"
                "</html>\n";
            std::ostringstream os;
            os << "HTTP/1.1 200 OK" << crlf                                                //
               << "Connection: close" << crlf                                              //
               << "Content-length: " << pstore::array_elements (quit_message) - 1U << crlf //
               << "Content-type: text/html" << crlf                                        //
               << "Server: pstore-httpd" << crlf                                           //
               << crlf // End of headers
               << quit_message;
            return pstore::httpd::send (sender, io, os.str ()) >>= fix_return_type;
        }


        template <typename Sender, typename IO>
        command_return_type<IO> handle_version (Sender sender, IO io, server_state state,
                                                query_container const & query) {
            std::string const version =
                "{ \"version\": \"0.1\" }"; // TODO: compute a proper version string.

            std::ostringstream os;
            os << "HTTP/1.1 200 OK" << crlf                       //
               << "Connection: close" << crlf                     //
               << "Content-length: " << version.length () << crlf //
               << "Content-type: application/json" << crlf        //
               << "Server: pstore-httpd" << crlf                  //
               << crlf                                            // End of headers
               << version;
            auto fix_return_type = [&state](IO io2) {
                return command_return_type<IO>{in_place, io2, state};
            };
            return pstore::httpd::send (sender, io, os.str ()) >>= fix_return_type;
        };


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
                using return_type = command_return_type<IO>;
                using function_type =
                    std::function<return_type (Sender, IO, server_state, query_container const &)>;

                // TODO: using unordered_map<> is lazy. The commands are known at compile-time so a
                // pre-sorted std::array<> which we can binary-chop would suffice.
                using container = std::unordered_map<std::string, function_type>;
            };

            template <typename Sender, typename IO>
            typename commands_helper<Sender, IO>::container const & get_commands () {
                static typename commands_helper<Sender, IO>::container const commands = {
                    {"quit", handle_quit<Sender, IO>},
                    {"version", handle_version<Sender, IO>},
                };
                return commands;
            }

        } // end namespace details


        template <typename Sender, typename IO>
        error_or<std::pair<IO, server_state>>
        serve_dynamic_content (Sender sender, IO io, std::string uri, server_state state) {
            using return_type = error_or<std::pair<IO, server_state>>;

            // Remove the common path prefix from the URI.
            assert (details::starts_with (uri, dynamic_path));
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
                                   pstore::httpd::make_insert_iterator (arguments));
            }

            // Do we know how to handle this command?
            auto const & commands = details::get_commands<Sender, IO> ();
            auto command_it = commands.find (uri.substr (0, pos));
            if (command_it == std::end (commands)) {
                return return_type{error_code::bad_request};
            }

            // Yep, this is a command we understand. Call it.
            return command_it->second (sender, io, state, arguments);
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_SERVE_DYNAMIC_CONTENT_HPP
