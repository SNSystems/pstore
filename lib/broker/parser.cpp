//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- lib/broker/parser.cpp ----------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file broker/parser.cpp

#include "pstore/broker/parser.hpp"

#include <cctype>
#include <mutex>

#include "pstore/broker/globals.hpp"

namespace {

    using iterator = std::string::const_iterator;
    using range = std::pair<iterator, iterator>;

    auto is_space = [] (char const c) {
        // std::isspace() has undefined behavior if the input value is not representable as unsigned
        // char and not not equal to EOF.
        auto const uc = static_cast<unsigned char> (c);
        return std::isspace (static_cast<int> (uc)) != 0;
    };

    iterator skip_ws (iterator first, iterator const last) {
        if (first != last && is_space (*first)) {
            ++first;
        }
        return first;
    }

    auto extract_word_no_ws (iterator const first, iterator const last) -> range {
        return {first, std::find_if (first, last, is_space)};
    }

    auto extract_word (iterator const first, iterator const last) -> range {
        return extract_word_no_ws (skip_ws (first, last), last);
    }

    auto substr (range const & range) -> std::string { return {range.first, range.second}; }


    auto extract_payload (pstore::brokerface::message_type const & msg)
        -> std::unique_ptr<std::string> {
        auto const pos = std::find_if (msg.payload.rbegin (), msg.payload.rend (),
                                       [] (char const c) { return c != '\0'; });
        return std::make_unique<std::string> (std::begin (msg.payload), pos.base ());
    }

} // end anonymous namespace


namespace pstore {
    namespace broker {

        std::unique_ptr<broker_command> parse (brokerface::message_type const & msg,
                                               partial_cmds & cmds) {

            if (msg.part_no >= msg.num_parts) {
                throw part_number_too_large ();
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
                throw number_of_parts_mismatch ();
            }

            bool const was_missing = value[msg.part_no].get () == nullptr;
            value[msg.part_no] = std::move (payload);

            if (was_missing) {
                auto not_null = [] (std::unique_ptr<std::string> const & str) {
                    return str != nullptr;
                };
                if (std::all_of (std::begin (value), std::end (value), not_null)) {
                    std::string complete_command;
                    for (auto const & c : value) {
                        PSTORE_ASSERT (c.get () != nullptr);
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
