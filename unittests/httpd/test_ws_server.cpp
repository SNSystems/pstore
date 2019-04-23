//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- unittests/httpd/test_ws_server.cpp ---------------------------------===//
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
#include "pstore/httpd/ws_server.hpp"

#include <gmock/gmock.h>

#include "buffered_reader_mocks.hpp"

using testing::_;
using testing::Invoke;

using pstore::httpd::make_buffered_reader;

namespace {}


TEST (WsServer, foo) {

    refiller r;
    EXPECT_CALL (r, fill (_, _)).WillRepeatedly (Invoke (eof ()));
    // EXPECT_CALL (r, fill (0, _)).WillOnce (Invoke (yield_string ("a")));

    std::vector<std::uint8_t> output;

    auto sender = [&output](int io, pstore::gsl::span<std::uint8_t const> const & s) {
        std::copy (s.begin (), s.end (), std::back_inserter (output));
        return pstore::error_or<int>{pstore::in_place, io + 1};
    };

    auto io = 0;
    auto br = make_buffered_reader<int> (r.refill_function ());

    ws_server_loop (br, sender, io);
}
