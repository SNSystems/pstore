//*                            *
//*   ___ _ __ _ __ ___  _ __  *
//*  / _ \ '__| '__/ _ \| '__| *
//* |  __/ |  | | | (_) | |    *
//*  \___|_|  |_|  \___/|_|    *
//*                            *
//===- lib/support/error.cpp ----------------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
/// \file support/error.cpp
/// \brief Provides an pstore-specific error codes and a suitable error category for them.

#include "pstore/support/error.hpp"

// ******************
// * error category *
// ******************
// name
// ~~~~
char const * pstore::error_category::name () const noexcept {
    return "pstore category";
}

// message
// ~~~~~~~
std::string pstore::error_category::message (int const error) const {
    static_assert (std::is_same<std::underlying_type<error_code>::type,
                                std::remove_cv<decltype (error)>::type>::value,
                   "base type of pstore::error_code must be int to permit safe static cast");

    auto * result = "unknown pstore::category error";
    switch (static_cast<error_code> (error)) {
    case error_code::none: result = "no error"; break;
    case error_code::transaction_on_read_only_database:
        result = "an attempt to create a transaction when the database is read-only";
        break;
    case error_code::unknown_revision: result = "unknown revision"; break;
    case error_code::header_corrupt: result = "header corrupt"; break;
    case error_code::header_version_mismatch: result = "header version mismatch"; break;
    case error_code::footer_corrupt: result = "footer corrupt"; break;
    case error_code::index_corrupt: result = "index corrupt"; break;
    case error_code::bad_alignment:
        result = "an address was not correctly aligned for its type";
        break;
    case error_code::index_not_latest_revision: result = "index not latest revision"; break;
    case error_code::unknown_process_path:
        result = "could not discover the path of the calling process image";
        break;
    case error_code::store_closed:
        result = "an attempt to read or write from a store which is not open";
        break;
    case error_code::cannot_allocate_after_commit:
        result = "an attempt was made to allocate data without an open transaction";
        break;
    case error_code::bad_address:
        result = "an attempt to address an address outside of the allocated storage";
        break;
    case error_code::read_only_address:
        result = "there was an attempt to write to read-only storage";
        break;
    case error_code::did_not_read_number_of_bytes_requested:
        result = "did not read number of bytes requested";
        break;
    case error_code::uuid_parse_error: result = "UUID parse error"; break;
    case error_code::bad_message_part_number: result = "bad message part number"; break;
    case error_code::unable_to_open_named_pipe: result = "unable to open named pipe"; break;
    case error_code::pipe_write_timeout: result = "pipe write timeout"; break;
    case error_code::write_failed: result = "write failed"; break;
    }
    return result;
}

// **********************
// * get_error_category *
// **********************
std::error_category const & pstore::get_error_category () {
    static pstore::error_category const cat;
    return cat;
}
