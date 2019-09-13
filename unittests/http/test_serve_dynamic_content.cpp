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
#include "pstore/http/serve_dynamic_content.hpp"
#include <iostream>

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>

#include "pstore/support/gsl.hpp"

TEST (ServeDynamicContent, BadRequest) {
    std::vector<std::uint8_t> output;
    auto sender = [&output](int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (std::begin (s), std::end (s), std::back_inserter (output));
        return pstore::error_or<int>{io};
    };

    int io = 0;
    pstore::error_or<int> const err = pstore::httpd::serve_dynamic_content (
        sender, io, std::string{pstore::httpd::dynamic_path} + "bad_request");
    EXPECT_EQ (err.get_error (), make_error_code (pstore::httpd::error_code::bad_request));
}

TEST (ServeDynamicContent, Version) {
    std::string output;
    auto sender = [&output](int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::transform (std::begin (s), std::end (s), std::back_inserter (output),
                        [](std::uint8_t v) { return static_cast<char> (v); });
        return pstore::error_or<int>{io};
    };

    pstore::error_or<int> const r = pstore::httpd::serve_dynamic_content (
        sender, 0, std::string{pstore::httpd::dynamic_path} + "version");
    EXPECT_TRUE (r);
    EXPECT_THAT (output, ::testing::ContainsRegex ("\r\n\r\n\\{ *\"version\" *:"));
}
