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
//===- include/pstore/httpd/serve_static_content.hpp ----------------------===//
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
#ifndef PSTORE_HTTPD_SERVE_STATIC_CONTENT_HPP
#define PSTORE_HTTPD_SERVE_STATIC_CONTENT_HPP

#include "pstore/httpd/http_date.hpp"
#include "pstore/httpd/media_type.hpp"
#include "pstore/httpd/send.hpp"
#include "pstore/romfs/romfs.hpp"

namespace pstore {
    namespace httpd {

        namespace details {

            template <typename Sender, typename IO>
            pstore::error_or<IO> read_and_send (Sender sender, IO io,
                                                pstore::romfs::descriptor fd) {
                std::array<std::uint8_t, 1024> buffer{{0}};
                auto data = buffer.data ();
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

            return file_system.stat (path.c_str ()) >>= [&](pstore::romfs::stat const & stat) {
                return file_system.open (path.c_str ()) >>= [&](pstore::romfs::descriptor fd) {
                    // Send the response header.
                    std::ostringstream os;
                    os << "HTTP/1.1 200 OK" << crlf << "Server: pstore-httpd" << crlf
                       << "Content-length: " << stat.st_size << crlf
                       << "Content-type: " << pstore::httpd::media_type_from_filename (path) << crlf
                       << "Date: " << http_date (std::chrono::system_clock::now ()) << crlf
                       << "Last-Modified: " << http_date (stat.st_mtime) << crlf << crlf;
                    return send (sender, io, os.str ()) >>=
                           [&](IO io2) { return details::read_and_send (sender, io2, fd); };
                };
            };
        }

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_SERVE_STATIC_CONTENT_HPP
