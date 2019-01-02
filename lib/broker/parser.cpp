//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- lib/broker/parser.cpp ----------------------------------------------===//
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
/// \file broker/parser.cpp

#include "pstore/broker/parser.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <mutex>
#include <stdexcept>

#include "pstore/broker/globals.hpp"
#include "pstore/broker_intf/message_type.hpp"
#include "pstore/support/make_unique.hpp"

namespace {

    using iterator = std::string::const_iterator;
    using range = std::pair<iterator, iterator>;

    auto is_space = [](char c) {
        // std::isspace() has undefined behavior if the input value is not representable as unsigned
        // char and not not equal to EOF.
        auto const uc = static_cast<unsigned char> (c);
        return std::isspace (static_cast<int> (uc)) != 0;
    };

    iterator skip_ws (iterator first, iterator last) {
        if (first != last && is_space (*first)) {
            ++first;
        }
        return first;
    }

    auto extract_word_no_ws (iterator first, iterator last) -> range {
        return {first, std::find_if (first, last, is_space)};
    }

    auto extract_word (iterator first, iterator last) -> range {
        return extract_word_no_ws (skip_ws (first, last), last);
    }

    auto substr (range const & range) -> std::string { return {range.first, range.second}; }


    auto extract_payload (pstore::broker::message_type const & msg)
        -> std::unique_ptr<std::string> {
        auto it = std::find_if (msg.payload.rbegin (), msg.payload.rend (),
                                [](char c) { return c != '\0'; });
        return std::make_unique<std::string> (std::begin (msg.payload), it.base ());
    }
} // end anonymous namespace


namespace pstore {
    namespace broker {

        std::unique_ptr<broker_command> parse (message_type const & msg, partial_cmds & cmds) {

            if (msg.part_no >= msg.num_parts) {
                throw std::runtime_error ("part number must be less than number of parts");
            }

            auto const now = std::chrono::system_clock::now ();

            auto payload = extract_payload (msg);

            auto const key = std::make_pair (msg.sender_id, msg.message_id);
            auto it = cmds.find (key);
            if (it == std::end (cmds)) {
                it = cmds.insert (std::make_pair (key, pieces{})).first;
                it->second.s_.resize (msg.num_parts);
            }

            // Record the arrival time of this newest message of the set.
            it->second.arrive_time_ = now;
            auto & value = it->second.s_;
            if (value.size () != msg.num_parts) {
                throw std::runtime_error ("total number of parts mismatch");
            }

            bool const was_missing = value[msg.part_no].get () == nullptr;
            value[msg.part_no] = std::move (payload);

            if (was_missing) {
                auto not_null = [](std::unique_ptr<std::string> const & str) {
                    return str.get () != nullptr;
                };
                if (std::all_of (std::begin (value), std::end (value), not_null)) {
                    std::string complete_command;
                    for (auto const & c : value) {
                        assert (c.get () != nullptr);
                        complete_command += *c;
                    }

                    cmds.erase (it);

                    auto end = std::end (complete_command);
                    auto const verb_parts = extract_word (std::begin (complete_command), end);
                    auto const path_parts = std::make_pair (skip_ws (verb_parts.second, end), end);
                    return std::make_unique<broker_command> (substr (verb_parts),
                                                             substr (path_parts));
                }
            }

            return nullptr;
        }

    } // end namespace broker
} // end namespace pstore
