//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===- include/pstore/broker/parser.hpp -----------------------------------===//
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
/// \file broker/parser.hpp

#ifndef PSTORE_BROKER_PARSER_HPP
#define PSTORE_BROKER_PARSER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pstore {
    namespace broker {
        struct message_type;

        struct broker_command {
        public:
            broker_command (std::string v, std::string p)
                    : verb{std::move (v)}
                    , path{std::move (p)} {}
            broker_command (broker_command const &) = default;
            broker_command (broker_command &&) = default;
            ~broker_command () noexcept = default;

            broker_command & operator= (broker_command const &) = default;
            broker_command & operator= (broker_command &&) = default;

            bool operator== (broker_command const & rhs) const {
                return verb == rhs.verb && path == rhs.path;
            }
            bool operator!= (broker_command const & rhs) const { return !operator== (rhs); }

            std::string verb;
            std::string path;
        };

        struct pieces {
            std::chrono::system_clock::time_point arrive_time_;
            std::vector<std::unique_ptr<std::string>> s_;
        };

        using size_pair = std::pair<std::size_t, std::size_t>;

    } // namespace broker
} // namespace pstore

// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {
    template <>
    struct hash<pstore::broker::size_pair> {
        std::size_t operator() (pstore::broker::size_pair const & p) const {
            auto h = std::hash<std::size_t>{};
            return h (p.first) ^ h (p.second);
        }
    };
} // namespace std

namespace pstore {
    namespace broker {

        using partial_cmds = std::unordered_map<size_pair, pieces>;

        std::unique_ptr<broker_command> parse (message_type const & msg, partial_cmds & cmds);

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_PARSER_HPP
