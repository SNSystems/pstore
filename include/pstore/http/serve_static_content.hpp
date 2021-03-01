//===- include/pstore/http/serve_static_content.hpp -------*- mode: C++ -*-===//
//*                                _        _   _       *
//*  ___  ___ _ ____   _____   ___| |_ __ _| |_(_) ___  *
//* / __|/ _ \ '__\ \ / / _ \ / __| __/ _` | __| |/ __| *
//* \__ \  __/ |   \ V /  __/ \__ \ || (_| | |_| | (__  *
//* |___/\___|_|    \_/ \___| |___/\__\__,_|\__|_|\___| *
//*                                                     *
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
#ifndef PSTORE_HTTP_SERVE_STATIC_CONTENT_HPP
#define PSTORE_HTTP_SERVE_STATIC_CONTENT_HPP

#include "pstore/http/http_date.hpp"
#include "pstore/http/media_type.hpp"
#include "pstore/http/send.hpp"
#include "pstore/romfs/romfs.hpp"

namespace pstore {
    namespace http {

        namespace details {

            template <typename Sender, typename IO>
            pstore::error_or<IO> read_and_send (Sender sender, IO io,
                                                pstore::romfs::descriptor fd) {
                std::array<std::uint8_t, 1024> buffer{{0}};
                auto * const data = buffer.data ();
                std::size_t const num_read =
                    fd.read (data, sizeof (decltype (buffer)::value_type), buffer.size ());
                if (num_read == 0) {
                    return pstore::error_or<IO>{io};
                }
                pstore::error_or<IO> const eo =
                    send (sender, io, gsl::span<std::uint8_t const> (data, data + num_read));
                if (!eo) {
                    return eo;
                }
                return read_and_send (sender, io, fd);
            }

        } // end namespace details

        template <typename Sender, typename IO>
        pstore::error_or<IO> serve_static_content (Sender sender, IO io, std::string path,
                                                   pstore::romfs::romfs const & file_system) {
            if (path.empty ()) {
                path = "/";
            }
            if (path.back () == '/') {
                path += "index.html";
            }

            return file_system.stat (path.c_str ()) >>= [&] (pstore::romfs::stat const & stat) {
                return file_system.open (path.c_str ()) >>= [&] (pstore::romfs::descriptor fd) {
                    // Send the response header.
                    std::ostringstream os;
                    os << "HTTP/1.0 200 OK" << crlf               //
                       << "Server: " << server_name << crlf       //
                       << "Content-length: " << stat.size << crlf //
                       << "Content-type: " << pstore::http::media_type_from_filename (path)
                       << crlf //
                       << "Connection: close"
                       << crlf // TODO remove this when we support persistent connections
                       << "Date: " << http_date (std::chrono::system_clock::now ()) << crlf //
                       << "Last-Modified: " << http_date (stat.mtime) << crlf               //
                       << crlf;
                    return send (sender, io, os.str ()) >>=
                           [&] (IO io2) { return details::read_and_send (sender, io2, fd); };
                };
            };
        }

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_SERVE_STATIC_CONTENT_HPP
