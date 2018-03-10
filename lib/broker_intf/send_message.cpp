//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- lib/broker_intf/send_message.cpp -----------------------------------===//
// Copyright (c) 2017-2018 by Sony Interactive Entertainment, Inc.
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
/// \file send_message.cpp

#include "pstore/broker_intf/send_message.hpp"

#include <iterator>
#include <string>

#include "pstore/broker_intf/message_type.hpp"
#include "pstore/broker_intf/writer.hpp"

namespace {
    static std::atomic<std::uint32_t> message_id{0};
}

namespace pstore {
    namespace broker {

        void send_message (writer & wr, bool error_on_timeout, ::pstore::gsl::czstring verb,
                           ::pstore::gsl::czstring path) {
            assert (verb != nullptr);

            auto payload = std::string{verb};
            if (path != nullptr && path[0] != '\0') {
                payload.append (" ");
                payload.append (path);
            }

            // Work out the number of pieces into which we need to break this payload.
            using num_parts_type = decltype (message_type::num_parts);
            auto const num_parts =
                static_cast<num_parts_type> ((payload.length () + message_type::payload_chars - 1) /
                                             message_type::payload_chars);

            std::uint32_t const mid = message_id++;

            // Build and send each message.
            auto first = std::begin (payload);

            using difference_type = std::iterator_traits<decltype (first)>::difference_type;
            static_assert (message_type::payload_chars <=
                               std::numeric_limits<difference_type>::max (),
                           "payload_chars is too large to be represented as "
                           "string::iterator::difference_type");

            for (auto part = num_parts_type{0}; part < num_parts; ++part) {
                auto remaining = std::distance (first, std::end (payload));
                assert (remaining > 0);

                auto last = first;
                std::advance (last, std::min (remaining, static_cast<difference_type> (
                                                             message_type::payload_chars)));

                wr.write (message_type{mid, part, num_parts, first, last}, error_on_timeout);
                first = last;
            }
        }

        std::uint32_t next_message_id () { return message_id.load (); }

    } // namespace broker
} // namespace pstore
// eof: lib/broker_intf/send_message.cpp
