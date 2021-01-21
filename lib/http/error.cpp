//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- lib/http/error.cpp -------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/error.hpp"

namespace pstore {
    namespace httpd {

        // ******************
        // * error category *
        // ******************
        // name
        // ~~~~
        gsl::czstring error_category::name () const noexcept { return "pstore httpd category"; }

        // message
        // ~~~~~~~
        std::string error_category::message (int const error) const {
            switch (static_cast<error_code> (error)) {
            case error_code::bad_request: return "Bad request";
            case error_code::bad_websocket_version: return "Bad WebSocket version requested";
            case error_code::not_implemented: return "Not implemented";
            case error_code::string_too_long: return "String too long";
            case error_code::refill_out_of_range: return "Refill result out of range";
            }
            return "unknown pstore::category error";
        }

        // **********************
        // * get_error_category *
        // **********************
        std::error_category const & get_error_category () noexcept {
            static error_category const cat;
            return cat;
        }

    } // end namespace httpd
} // end namespace pstore
