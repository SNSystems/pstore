//*                                _                             _       *
//*  ___  ___ _ ____   _____    __| |_   _ _ __   __ _ _ __ ___ (_) ___  *
//* / __|/ _ \ '__\ \ / / _ \  / _` | | | | '_ \ / _` | '_ ` _ \| |/ __| *
//* \__ \  __/ |   \ V /  __/ | (_| | |_| | | | | (_| | | | | | | | (__  *
//* |___/\___|_|    \_/ \___|  \__,_|\__, |_| |_|\__,_|_| |_| |_|_|\___| *
//*                                  |___/                               *
//*                  _             _    *
//*   ___ ___  _ __ | |_ ___ _ __ | |_  *
//*  / __/ _ \| '_ \| __/ _ \ '_ \| __| *
//* | (_| (_) | | | | ||  __/ | | | |_  *
//*  \___\___/|_| |_|\__\___|_| |_|\__| *
//*                                     *
//===- unittests/http/test_serve_dynamic_content.cpp ----------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
#include "pstore/http/serve_dynamic_content.hpp"
#include <iostream>

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>

#include "pstore/support/gsl.hpp"

TEST (ServeDynamicContent, BadRequest) {
    std::vector<std::uint8_t> output;
    auto sender = [&output] (int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (std::begin (s), std::end (s), std::back_inserter (output));
        return pstore::error_or<int>{io};
    };

    int io = 0;
    pstore::error_or<int> const err = pstore::http::serve_dynamic_content (
        sender, io, std::string{pstore::http::dynamic_path} + "bad_request");
    EXPECT_EQ (err.get_error (), make_error_code (pstore::http::error_code::bad_request));
}

TEST (ServeDynamicContent, Version) {
    std::string output;
    auto sender = [&output] (int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::transform (std::begin (s), std::end (s), std::back_inserter (output),
                        [] (std::uint8_t v) { return static_cast<char> (v); });
        return pstore::error_or<int>{io};
    };

    pstore::error_or<int> const r = pstore::http::serve_dynamic_content (
        sender, 0, std::string{pstore::http::dynamic_path} + "version");
    EXPECT_TRUE (r);
    EXPECT_THAT (output, ::testing::ContainsRegex ("\r\n\r\n\\{ *\"version\" *:"));
}
