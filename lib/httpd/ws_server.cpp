//*                                               *
//* __      _____   ___  ___ _ ____   _____ _ __  *
//* \ \ /\ / / __| / __|/ _ \ '__\ \ / / _ \ '__| *
//*  \ V  V /\__ \ \__ \  __/ |   \ V /  __/ |    *
//*   \_/\_/ |___/ |___/\___|_|    \_/ \___|_|    *
//*                                               *
//===- lib/httpd/ws_server.cpp --------------------------------------------===//
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

namespace pstore {
    namespace httpd {

        char const * ws_error_category::name () const noexcept { return "ws-error"; }

        std::string ws_error_category::message (int error) const {
            switch (static_cast<ws_error> (error)) {
            case ws_error::reserved_bit_set:
                return "One of a client frame's reserved bits was unexpectedly set";
            case ws_error::payload_too_long: return "The frame's payload length was too large";
            case ws_error::unmasked_frame: return "The client sent an unmasked frame";
            case ws_error::message_too_long: return "Message too long";
            }
            return "Unknown error";
        }

        std::error_code make_error_code (ws_error e) {
            static_assert (std::is_same<std::underlying_type<decltype (e)>::type, int>::value,
                           "base type of ws_error must be int to permit safe static cast");
            static ws_error_category category;
            return {static_cast<int> (e), category};
        }


    } // end namespace httpd
} // end namespace pstore
