//*              _ _    *
//*   __ _ _   _(_) |_  *
//*  / _` | | | | | __| *
//* | (_| | |_| | | |_  *
//*  \__, |\__,_|_|\__| *
//*     |_|             *
//===- include/pstore/http/quit.hpp ---------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#ifndef PSTORE_HTTP_QUIT_HPP
#define PSTORE_HTTP_QUIT_HPP

#include "pstore/http/server_status.hpp"
#include "pstore/support/gsl.hpp"
#include "pstore/support/maybe.hpp"

namespace pstore {
    namespace http {

        void quit (gsl::not_null<maybe<server_status> *> http_status);

    } // end namespace http
} // end namespace pstore

#endif // PSTORE_HTTP_QUIT_HPP
