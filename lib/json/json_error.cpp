//*    _                                             *
//*   (_)___  ___  _ __     ___ _ __ _ __ ___  _ __  *
//*   | / __|/ _ \| '_ \   / _ \ '__| '__/ _ \| '__| *
//*   | \__ \ (_) | | | | |  __/ |  | | | (_) | |    *
//*  _/ |___/\___/|_| |_|  \___|_|  |_|  \___/|_|    *
//* |__/                                             *
//===- lib/json/json_error.cpp --------------------------------------------===//
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
#include "pstore/json/json_error.hpp"

// ******************
// * error category *
// ******************
pstore::json::error_category::error_category () noexcept = default;

char const * pstore::json::error_category::name () const noexcept {
    return "json parser category";
}

std::string pstore::json::error_category::message (int const error) const {
    auto * result = "unknown json::error_category error";
    switch (static_cast<error_code> (error)) {
    case error_code::none: result = "none"; break;
    case error_code::bad_unicode_code_point: result = "bad UNICODE code point"; break;
    case error_code::expected_array_member: result = "expected array member"; break;
    case error_code::expected_close_quote: result = "expected close quote"; break;
    case error_code::expected_colon: result = "expected colon"; break;
    case error_code::expected_digits: result = "expected digits"; break;
    case error_code::expected_object_member: result = "expected object member"; break;
    case error_code::expected_string: result = "expected string"; break;
    case error_code::expected_token: result = "expected token"; break;
    case error_code::invalid_escape_char: result = "invalid escape character"; break;
    case error_code::invalid_hex_char: result = "invalid hexadecimal escape character"; break;
    case error_code::number_out_of_range: result = "number out of range"; break;
    case error_code::unexpected_extra_input: result = "unexpected extra input"; break;
    case error_code::unrecognized_token: result = "unrecognized token"; break;
    case error_code::nesting_too_deep: result = "objects are too deeply nested"; break;
    }
    return result;
}

std::error_category const & pstore::json::get_error_category () noexcept {
    static pstore::json::error_category const cat;
    return cat;
}
