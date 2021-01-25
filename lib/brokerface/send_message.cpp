//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- lib/brokerface/send_message.cpp ------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file send_message.cpp

#include "pstore/brokerface/send_message.hpp"

#include <iterator>
#include <string>

#include "pstore/brokerface/message_type.hpp"
#include "pstore/brokerface/writer.hpp"

namespace {

    std::atomic<std::uint32_t> message_id{0};

} // end anonymous namespace

namespace pstore {
    namespace brokerface {

        void send_message (writer & wr, bool const error_on_timeout, gsl::czstring const verb,
                           gsl::czstring const path) {
            PSTORE_ASSERT (verb != nullptr);

            auto payload = std::string{verb};
            if (path != nullptr && path[0] != '\0') {
                payload.append (" ");
                payload.append (path);
            }

            // Work out the number of pieces into which we need to break this payload.
            using num_parts_type = std::remove_const<decltype (message_type::num_parts)>::type;
            auto const num_parts =
                static_cast<num_parts_type> ((payload.length () + message_type::payload_chars - 1) /
                                             message_type::payload_chars);

            std::uint32_t const mid = message_id++;

            // Build and send each message.
            auto first = std::begin (payload);

            using difference_type = std::iterator_traits<decltype (first)>::difference_type;
            static_assert (message_type::payload_chars <=
                               static_cast<std::make_unsigned<difference_type>::type> (
                                   std::numeric_limits<difference_type>::max ()),
                           "payload_chars is too large to be represented as "
                           "string::iterator::difference_type");

            for (auto part = num_parts_type{0}; part < num_parts; ++part) {
                auto const remaining = std::distance (first, std::end (payload));
                PSTORE_ASSERT (remaining > 0);

                auto last = first;
                std::advance (last, std::min (remaining, static_cast<difference_type> (
                                                             message_type::payload_chars)));

                wr.write (message_type{mid, part, num_parts, first, last}, error_on_timeout);
                first = last;
            }
        }

        std::uint32_t next_message_id () { return message_id.load (); }

    } // end namespace brokerface
} // end namespace pstore
