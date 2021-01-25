//*  _                    _                *
//* | |__   ___  __ _  __| | ___ _ __ ___  *
//* | '_ \ / _ \/ _` |/ _` |/ _ \ '__/ __| *
//* | | | |  __/ (_| | (_| |  __/ |  \__ \ *
//* |_| |_|\___|\__,_|\__,_|\___|_|  |___/ *
//*                                        *
//===- include/pstore/http/headers.hpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_HEADERS_HPP
#define PSTORE_HTTP_HEADERS_HPP

#include <string>

#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace http {

        struct header_info {
            bool operator== (header_info const & rhs) const;
            bool operator!= (header_info const & rhs) const { return !operator== (rhs); }

            bool upgrade_to_websocket = false;
            bool connection_upgrade = false;
            pstore::maybe<std::string> websocket_key;
            pstore::maybe<unsigned> websocket_version;

            header_info handler (std::string const & key, std::string const & value);
        };

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_HEADERS_HPP
