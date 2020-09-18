//*                                           _    *
//*   ___ ___  _ __ ___  _ __ ___   ___ _ __ | |_  *
//*  / __/ _ \| '_ ` _ \| '_ ` _ \ / _ \ '_ \| __| *
//* | (_| (_) | | | | | | | | | | |  __/ | | | |_  *
//*  \___\___/|_| |_| |_|_| |_| |_|\___|_| |_|\__| *
//*                                                *
//===- unittests/json/test_comment.cpp ------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
#include "pstore/json/json.hpp"

#include "callbacks.hpp"

using namespace std::string_literals;

namespace {

    class JsonComment : public ::testing::Test {
    public:
        JsonComment ()
                : proxy_{callbacks_} {}

    protected:
        testing::StrictMock<mock_json_callbacks> callbacks_;
        callbacks_proxy<mock_json_callbacks> proxy_;
    };

} // end anonymous namespace

TEST_F (JsonComment, Disabled) {
    pstore::json::parser<decltype (proxy_)> p = pstore::json::make_parser (proxy_);
    p.input ("# comment\nnull"s).eof ();
    EXPECT_TRUE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (JsonComment, SingleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    pstore::json::parser<decltype (proxy_)> p =
        pstore::json::make_parser (proxy_, pstore::json::parser_extensions::bash_comments);
    p.input ("# comment\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (JsonComment, MultipleLeading) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    pstore::json::parser<decltype (proxy_)> p =
        pstore::json::make_parser (proxy_, pstore::json::parser_extensions::bash_comments);
    p.input ("# comment\n\n    # remark\nnull"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (JsonComment, Trailing) {
    EXPECT_CALL (callbacks_, null_value ()).Times (1);

    pstore::json::parser<decltype (proxy_)> p =
        pstore::json::make_parser (proxy_, pstore::json::parser_extensions::bash_comments);
    p.input ("null # comment"s).eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}

TEST_F (JsonComment, InsideArray) {
    using testing::_;
    EXPECT_CALL (callbacks_, begin_array ()).Times (1);
    EXPECT_CALL (callbacks_, uint64_value (_)).Times (2);
    EXPECT_CALL (callbacks_, end_array ()).Times (1);

    pstore::json::parser<decltype (proxy_)> p =
        pstore::json::make_parser (proxy_, pstore::json::parser_extensions::bash_comments);
    p.input (R"([#comment
1,     # comment containing #
2 # comment
]
)"s)
        .eof ();
    EXPECT_FALSE (p.has_error ()) << "JSON error was: " << p.last_error ().message ();
}
