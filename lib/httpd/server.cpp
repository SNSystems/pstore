//*                                *
//*  ___  ___ _ ____   _____ _ __  *
//* / __|/ _ \ '__\ \ / / _ \ '__| *
//* \__ \  __/ |   \ V /  __/ |    *
//* |___/\___|_|    \_/ \___|_|    *
//*                                *
//===- lib/httpd/server.cpp -----------------------------------------------===//
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
#include "pstore/httpd/server.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <iostream>

#ifdef _WIN32

#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#    include <io.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>

#else

#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/mman.h>
#    include <sys/socket.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>

#endif

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/httpd/buffered_reader.hpp"
#include "pstore/httpd/media_type.hpp"
#include "pstore/httpd/query_to_kvp.hpp"
#include "pstore/support/error.hpp"


// FIXME: there are multiple definitions of this type.
#ifdef _WIN32
using ssize_t = SSIZE_T;
#endif

namespace {

    void error (char const * msg) { std::cerr << msg << '\n'; }

    void send (pstore::broker::socket_descriptor const & socket, void const * ptr,
               std::size_t size) {
        ::send (socket.get (), static_cast<char const *> (ptr), size, 0 /*flags*/);
    }
    void send (pstore::broker::socket_descriptor const & socket, std::string const & str) {
        // FIXME: error handling.
        send (socket, str.data (), str.length ());
    }
    void send (pstore::broker::socket_descriptor const & socket, std::ostringstream const & os) {
        send (socket, os.str ());
    }
    void cerror (pstore::broker::socket_descriptor const & socket, char const * cause,
                 char const * error_no, char const * shortmsg, char const * longmsg) {
        std::ostringstream os;
        os << "HTTP/1.1 " << error_no << ' ' << shortmsg
           << "\n"
              "Content-type: text/html\n"
              "\n"
              "<html>\n"
              "<head><title>pstore-httpd Error</title></head>\n"
              "<body>\n"
              "<h1>pstore-httpd Web Server Error</h1>\n"
              "<p>"
           << error_no << ": " << shortmsg
           << "</p>\n"
              "<p>"
           << longmsg << ": " << cause
           << "</p>\n"
              "<hr><em>The pstore-httpd Web server</em>\n"
              "</body>\n"
              "</html>\n";
        send (socket, os);
    }

    // FIXME: the following two functions were copied from the broker.
    // is_recv_error
    // ~~~~~~~~~~~~~
    constexpr bool is_recv_error (ssize_t nread) {
#ifdef _WIN32
        return nread == SOCKET_ERROR;
#else
        return nread < 0;
#endif // !_WIN32
    }

    // get_last_error
    // ~~~~~~~~~~~~~~
    inline std::error_code get_last_error () noexcept {
#ifdef _WIN32
        return std::make_error_code (pstore::win32_erc{static_cast<DWORD> (WSAGetLastError ())});
#else
        return std::make_error_code (std::errc (errno));
#endif
    }


    class refiller {
    public:
        explicit constexpr refiller (pstore::broker::socket_descriptor * const socket) noexcept
                : socket_{socket} {}
        constexpr refiller (refiller const & rhs) noexcept = default;
        constexpr refiller (refiller && rhs) noexcept = default;

        refiller & operator= (refiller const &) noexcept = default;

        pstore::error_or<char *> operator() (char * first, char * last);

    private:
        pstore::broker::socket_descriptor * socket_;
    };
    pstore::error_or<char *> refiller::operator() (char * first, char * last) {
        assert (last > first);
        ssize_t const nread =
            recv (socket_->get (), first, static_cast<std::size_t> (last - first), 0 /*flags*/);
        assert (is_recv_error (nread) || nread <= last - first);
        if (is_recv_error (nread)) {
            return pstore::error_or<char *>{get_last_error ()};
        }
        return pstore::error_or<char *>{first + nread};
    }

} // end anonymous namespace

namespace pstore {
    namespace httpd {

        using socket_descriptor = pstore::broker::socket_descriptor;

        int server (in_port_t port_number, romfs::romfs & file_system) {
            // open socket descriptor.
            socket_descriptor parentfd{::socket (AF_INET, SOCK_STREAM, 0)};
            if (!parentfd.valid ()) {
                error ("ERROR opening socket");
            }

            {
                // Allows us to restart the server immediately.
                int const optval = 1;
                setsockopt (parentfd.get (), SOL_SOCKET, SO_REUSEADDR,
                            reinterpret_cast<char const *> (&optval), sizeof (optval));
            }

            {
                // Bind port to socket.
                sockaddr_in serveraddr; /* server's addr */
                std::memset (&serveraddr, 0, sizeof (serveraddr));
                serveraddr.sin_family = AF_INET;
                serveraddr.sin_addr.s_addr = htonl (INADDR_ANY);
                serveraddr.sin_port = htons (port_number);
                if (bind (parentfd.get (), reinterpret_cast<sockaddr *> (&serveraddr),
                          sizeof (serveraddr)) < 0) {
                    error ("ERROR on binding");
                }
            }

            /* get us ready to accept connection requests */
            // FIXMW: socket_error on win32
            if (listen (parentfd.get (), 5) < 0) { /* allow 5 requests to queue up */
                error ("ERROR on listen");
            }

            /*
             * main loop: wait for a connection request, parse HTTP,
             * serve requested content, close connection.
             */
            sockaddr_in clientaddr; // client address.
            auto clientlen =
                static_cast<socklen_t> (sizeof (clientaddr)); // byte size of client's address
            for (;;) {

                // Wait for a connection request.
                // TODO: some kind of shutdown procedure.
                socket_descriptor childfd{
                    ::accept (parentfd.get (), reinterpret_cast<struct sockaddr *> (&clientaddr),
                              &clientlen)};
                if (!childfd.valid ()) {
                    error ("ERROR on accept");
                }

                // Determine who sent the message.
                hostent * const hostp =
                    gethostbyaddr (reinterpret_cast<char const *> (&clientaddr.sin_addr.s_addr),
                                   sizeof (clientaddr.sin_addr.s_addr), AF_INET);
                if (hostp == nullptr) {
                    error ("ERROR on gethostbyaddr");
                }
                char const * hostaddrp = inet_ntoa (clientaddr.sin_addr);
                if (hostaddrp == nullptr) {
                    error ("ERROR on inet_ntoa\n");
                }

                auto reader = make_buffered_reader (refiller (&childfd));

                /* get the HTTP request line */
                constexpr std::size_t buffer_size = 1024;
                char method[buffer_size];  // request method
                char version[buffer_size]; // request method
                char uri[buffer_size];     // request uri
                {
                    // char buf[buffer_size]; // message buffer
                    error_or<maybe<std::string>> ebuf = reader.gets ();
                    if (ebuf.has_error ()) {
                        std::error_code const erc = ebuf.get_error ();
                        std::cerr << "Error on recv: " << erc << '\n';
                        // childfd.reset ();
                        continue;
                    }
                    maybe<std::string> buf = ebuf.get_value ();
                    if (!buf) {
                        continue;
                    }
                    // fgets (buf, buffer_size, stream);
                    std::cout << *buf << '\n';
                    sscanf (buf->c_str (), "%s %s %s\n", method, uri, version);
                }

                // We  only currently support the GET method.
                if (strcmp (method, "GET") != 0) {
                    cerror (childfd, method, "501", "Not Implemented",
                            "httpd does not implement this method");

                    childfd.reset ();
                    continue;
                }

                /* read (and ignore) the HTTP headers */
                {
                    // char buf[buffer_size]; /* message buffer */
                    std::string s;
                    do {
                        error_or<maybe<std::string>> es = reader.gets ();
                        s = es.get_value ().value ();
                        std::cout << s << '\n';
                    } while (s.length () > 0);
                }

                /* parse the uri [crufty] */
                bool is_static = true; /* static request? */
                std::string filename;
                char cgiargs[buffer_size];      /* cgi argument list */
                if (!strstr (uri, "cgi-bin")) { /* static content */
                    is_static = true;
                    cgiargs[0] = '\0';
                    filename = ".";
                    filename += uri;
                    if (uri[std::strlen (uri) - 1] == '/') {
                        filename += "index.html";
                    }
                } else { /* dynamic content */
#if 0
                    is_static = false;
                    if (char * p = index (uri, '?')) {
                        std::strcpy (cgiargs, p + 1);
                        *p = '\0';
                    } else {
                        std::strcpy (cgiargs, "");
                    }
                    filename = ".";
                    filename += uri;
#endif
                }

                /* make sure the file exists */
                // struct stat sbuf; /* file status */
                error_or<struct romfs::stat> sbuf = file_system.stat (filename.c_str ());
                if (sbuf.has_error ()) {
                    cerror (childfd, filename.c_str (), "404", "Not found",
                            "pstore-httpd couldn't find this file");

                    childfd.reset ();
                    continue;
                }

                /* serve static content */
                if (is_static) {
                    /* print response header */
                    //auto const * reply = "<html><body>Hello</body></html>";
                    //auto const content_length = std::strlen (reply);
                    {
                        std::ostringstream os;
                        os << "HTTP/1.1 200 OK\nServer: pstore-httpd\n";
                        os << "Content-length: " << static_cast<unsigned> (sbuf->st_size) << '\n';
                        os << "Content-type: " << media_type_from_filename (filename) << '\n';
                        os << "\r\n";
                        send (childfd, os.str ());
                    }
                    std::array <std::uint8_t, 256> buffer;
                    error_or<romfs::descriptor> descriptor = file_system.open (filename.c_str ());
                    assert (!descriptor.has_error ());
                    for (;;) {
                        std::size_t num_read = descriptor->read (buffer.data (), sizeof (std::uint8_t), buffer.size ());
                        if (num_read == 0) {
                            break;
                        }
                        send (childfd, buffer.data (), num_read);
                    }
                } else {
#if 0
                    /* serve dynamic content */

                    /* make sure file is a regular executable file */
                    if (!(S_IFREG & sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
                        cerror (stream, filename.c_str (), "403", "Forbidden",
                                "You are not allowed to access this item");
                        std::fclose (stream);
                        close (childfd);
                        continue;
                    }

                    /* a real server would set other CGI environ vars as well*/
                    setenv ("QUERY_STRING", cgiargs, 1);

                    {
                        // print first part of response header
                        char const header1[] = "HTTP/1.1 200 OK\nServer: pstore-httpd\n";
                        write (childfd, header1, sizeof (header1) - 1);
                    }

                    /* create and run the child CGI process so that all child
                       output to stdout and stderr goes back to the client via the
                       childfd socket descriptor */
                    int pid = fork ();
                    if (pid < 0) {
                        perror ("ERROR in fork");
                        std::exit (1);
                    } else if (pid > 0) {
                        /* parent process */
                        int wait_status; /* status from wait */
                        wait (&wait_status);
                    } else {
                        /* child  process*/
                        close (0);              /* close stdin */
                        dup2 (childfd, 1);      /* map socket to stdout */
                        dup2 (childfd, 2);      /* map socket to stderr */
                        extern char ** environ; /* the environment */
                        if (execve (filename.c_str (), NULL, environ) < 0) {
                            perror ("ERROR in execve");
                        }
                    }
#endif
                }

                /* clean up */
                childfd.reset ();
            }
        }

    } // end namespace httpd
} // end namespace pstore
