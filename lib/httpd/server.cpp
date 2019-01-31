#include "pstore/httpd/server.hpp"

#include <cstdio>
#include <cstring>
#include <string>

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
#include "pstore/httpd/media_type.hpp"
#include "pstore/httpd/query_to_kvp.hpp"

void error (char const *) {}
/*
 * cerror - returns an error message to the client
 */
void cerror (FILE * const stream, char const * cause, char const * error_no, char const * shortmsg,
             char const * longmsg) {
    std::fprintf (stream, "HTTP/1.1 %s %s\n", error_no, shortmsg);
    std::fprintf (stream, "Content-type: text/html\n");
    std::fprintf (stream, "\n");
    std::fprintf (stream, "<html><title>pstore-httpd Error</title>\n");
    std::fprintf (stream, "<body bgcolor=ffffff>\n");
    std::fprintf (stream, "<h1>pstore-httpd Web Server Error</h1>\n");
    std::fprintf (stream, "<p>%s: %s\n", error_no, shortmsg);
    std::fprintf (stream, "<p>%s: %s\n", longmsg, cause);
    std::fprintf (stream, "<hr><em>The pstore-httpd Web server</em>\n");
    std::fprintf (stream, "</body>\n");
}

namespace {


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

                // Open the child socket descriptor as a stream.
                FILE * const stream = fdopen (childfd.get (), "r+"); /* stream version of childfd */
                if (stream == nullptr) {
                    error ("ERROR on fdopen");
                }

                /* get the HTTP request line */
                constexpr std::size_t buffer_size = 1024;
                char method[buffer_size];  // request method
                char version[buffer_size]; // request method
                char uri[buffer_size];     // request uri
                {
                    char buf[buffer_size]; // message buffer
                    fgets (buf, buffer_size, stream);
                    std::printf ("%s", buf);
                    sscanf (buf, "%s %s %s\n", method, uri, version);
                }

                // We  only currently support the GET method.
                if (strcmp (method, "GET") != 0) {
                    cerror (stream, method, "501", "Not Implemented",
                            "httpd does not implement this method");
                    std::fclose (stream);
                    childfd.reset ();
                    continue;
                }

                /* read (and ignore) the HTTP headers */
                {
                    char buf[buffer_size]; /* message buffer */
                    fgets (buf, buffer_size, stream);
                    std::printf ("%s", buf);
                    while (std::strcmp (buf, "\r\n") != 0) {
                        std::fgets (buf, buffer_size, stream);
                        std::printf ("%s", buf);
                    }
                }

                /* parse the uri [crufty] */
                bool is_static = true; /* static request? */
                std::string filename;
                char cgiargs[buffer_size];      /* cgi argument list */
                if (!strstr (uri, "cgi-bin")) { /* static content */
                    is_static = true;
                    std::strcpy (cgiargs, "");
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
                romfs::error_or<struct romfs::stat> sbuf = file_system.stat (filename.c_str ());
                if (sbuf.has_error ()) {
                    cerror (stream, filename.c_str (), "404", "Not found",
                            "pstore-httpd couldn't find this file");
                    std::fclose (stream);
                    childfd.reset ();
                    continue;
                }

                /* serve static content */
                if (is_static) {
                    /* print response header */
                    //auto const * reply = "<html><body>Hello</body></html>";
                    //auto const content_length = std::strlen (reply);

                    std::fprintf (stream, "HTTP/1.1 200 OK\nServer: pstore-httpd\n");
                    // std::fprintf (stream, "Content-length: %d\n", static_cast<int>
                    // (sbuf->st_size));
                    std::fprintf (stream, "Content-length: %u\n",
                                  static_cast<unsigned> (sbuf->st_size));
                    std::fprintf (stream, "Content-type: %s\n",
                                  media_type_from_filename (filename));
                    std::fprintf (stream, "\r\n");
                    std::fflush (stream);

                    std::array <std::uint8_t, 256> buffer;
                    romfs::error_or<romfs::descriptor> descriptor = file_system.open (filename.c_str ());
                    assert (!descriptor.has_error ());
                    for (;;) {
                        std::size_t num_read = descriptor->read (buffer.data (), sizeof (std::uint8_t), buffer.size ());
                        if (num_read == 0) {
                            break;
                        }
                        std::fwrite (buffer.data (), sizeof(std::uint8_t), num_read, stream);
                    }
                    /* Use mmap to return arbitrary-sized response body */
                    //                    int fd = open (filename.c_str (), O_RDONLY);
                    //                    void * p = mmap (0, sbuf.st_size, PROT_READ, MAP_PRIVATE,
                    //                    fd, 0); std::fwrite (p, 1, sbuf.st_size, stream); munmap
                    //                    (p, sbuf.st_size); close (fd);
                    //std::fwrite (reply, 1, content_length, stream);
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
                std::fclose (stream);
                childfd.reset ();
            }
        }

    } // end namespace httpd
} // end namespace pstore
