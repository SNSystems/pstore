//
//  server.hpp
//  pstore
//
//  Created by Paul Bowen-Huggett on 28/01/2019.
//

#ifndef PSTORE_HTTPD_SERVER_HPP
#define PSTORE_HTTPD_SERVER_HPP

#ifdef _WIN32
using in_port_t = unsigned short;
#else
#    include <netinet/in.h>
#endif

#include "pstore/romfs/romfs.hpp"

namespace pstore {
    namespace httpd {

        int server (in_port_t port_number, romfs::romfs & file_system);

    } // end namespace httpd
} // end namespace pstore

#endif // PSTORE_HTTPD_SERVER_HPP
