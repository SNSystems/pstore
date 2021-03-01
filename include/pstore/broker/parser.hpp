//===- include/pstore/broker/parser.hpp -------------------*- mode: C++ -*-===//
//*                                  *
//*  _ __   __ _ _ __ ___  ___ _ __  *
//* | '_ \ / _` | '__/ __|/ _ \ '__| *
//* | |_) | (_| | |  \__ \  __/ |    *
//* | .__/ \__,_|_|  |___/\___|_|    *
//* |_|                              *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file broker/parser.hpp

#ifndef PSTORE_BROKER_PARSER_HPP
#define PSTORE_BROKER_PARSER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pstore/brokerface/message_type.hpp"

namespace pstore {
    namespace broker {

        class part_number_too_large : public std::runtime_error {
        public:
            part_number_too_large ()
                    : std::runtime_error (
                          "message part number must be less than the number of parts") {}
        };

        class number_of_parts_mismatch : public std::runtime_error {
        public:
            number_of_parts_mismatch ()
                    : std::runtime_error ("total number of parts mismatch") {}
        };


        class broker_command {
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

    } // end namespace broker
} // end namespace pstore

// NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std {
    template <>
    struct hash<pstore::broker::size_pair> {
        std::size_t operator() (pstore::broker::size_pair const & p) const {
            auto const h = std::hash<std::size_t>{};
            return h (p.first) ^ h (p.second);
        }
    };
} // end namespace std

namespace pstore {
    namespace broker {

        using partial_cmds = std::unordered_map<size_pair, pieces>;

        std::unique_ptr<broker_command> parse (brokerface::message_type const & msg,
                                               partial_cmds & cmds);

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_PARSER_HPP
