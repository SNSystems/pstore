//*    _                                             *
//*   (_)___  ___  _ __     ___ _ __ _ __ ___  _ __  *
//*   | / __|/ _ \| '_ \   / _ \ '__| '__/ _ \| '__| *
//*   | \__ \ (_) | | | | |  __/ |  | | | (_) | |    *
//*  _/ |___/\___/|_| |_|  \___|_|  |_|  \___/|_|    *
//* |__/                                             *
//===- lib/json/json_error.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
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
