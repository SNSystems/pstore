//*                                _        _   _       *
//*  ___  ___ _ ____   _____   ___| |_ __ _| |_(_) ___  *
//* / __|/ _ \ '__\ \ / / _ \ / __| __/ _` | __| |/ __| *
//* \__ \  __/ |   \ V /  __/ \__ \ || (_| | |_| | (__  *
//* |___/\___|_|    \_/ \___| |___/\__\__,_|\__|_|\___| *
//*                                                     *
//*                  _             _    *
//*   ___ ___  _ __ | |_ ___ _ __ | |_  *
//*  / __/ _ \| '_ \| __/ _ \ '_ \| __| *
//* | (_| (_) | | | | ||  __/ | | | |_  *
//*  \___\___/|_| |_|\__\___|_| |_|\__| *
//*                                     *
//===- unittests/httpd/test_serve_static_content.cpp ----------------------===//
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
#include "pstore/httpd/serve_static_content.hpp"

#include <algorithm>
#include <array>

#include "pstore/support/array_elements.hpp"

#include <gtest/gtest.h>

namespace {

    char const file1[] = "<!DOCTYPE html><html></html>";
    static constexpr std::size_t file1_size = pstore::array_elements (file1) - 1U;

    extern pstore::romfs::directory const root_dir;
    std::array<pstore::romfs::dirent, 3> const root_dir_membs = {{
        {".", &root_dir},
        {"..", &root_dir},
        {"index.html", reinterpret_cast<std::uint8_t const *> (file1), file1_size, 0},
    }};
    pstore::romfs::directory const root_dir{root_dir_membs};
    pstore::romfs::directory const * const root = &root_dir;

} // end anonymous namespace

namespace {

    class ServeStaticContent : public ::testing::Test {
    public:
        ServeStaticContent ()
                : fs_{root} {}

    protected:
        pstore::romfs::romfs & fs () noexcept { return fs_; }

    private:
        pstore::romfs::romfs fs_;
    };

} // end anonymous namespace


TEST_F (ServeStaticContent, Simple) {
    std::ostringstream str;
    str << "HTTP/1.1 200 OK\r\nServer: pstore-httpd\r\n"
           "Content-length: "
        << file1_size
        << "\r\n"
           "Content-type: text/html\r\n\r\n"
        << file1;
    std::string const expected = str.str ();

    std::string actual;
    pstore::error_or<int> const err = pstore::httpd::serve_static_content (
        [&actual](int io, pstore::gsl::span<std::uint8_t const> const & sp) {
            std::transform (std::begin (sp), std::end (sp), std::back_inserter (actual),
                            [](std::uint8_t b) { return static_cast<char> (b); });
            return pstore::error_or<int> (io + 1);
        },
        0, "/index.html", fs ());
    ASSERT_TRUE (static_cast<bool> (err));
    EXPECT_EQ (actual, expected);
}
