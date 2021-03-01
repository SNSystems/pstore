//===- lib/brokerface/message_type.cpp ------------------------------------===//
//*                                             _                     *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | __| | | | '_ \ / _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__|\__, | .__/ \___| *
//*                                |___/            |___/|_|          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file message_type.cpp
/// \brief Contains the definition of the broker::message_type class.

#include "pstore/brokerface/message_type.hpp"

#include <iterator>

#ifdef _WIN32
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    include <unistd.h>
#endif

#ifdef _WIN32

namespace {

    inline DWORD getpid () { return ::GetCurrentProcessId (); }

} // end anonymous namespace

#endif

namespace pstore {
    namespace brokerface {

        constexpr std::size_t message_type::payload_chars;

        std::uint32_t const message_type::process_id = static_cast<std::uint32_t> (getpid ());

        // (ctor)
        // ~~~~~~
        message_type::message_type (std::uint32_t const mid, std::uint16_t const pno,
                                    std::uint16_t const nump, std::string const & content)
                : message_type (mid, pno, nump, std::begin (content), std::end (content)) {}

        // operator==
        // ~~~~~~~~~~
        bool message_type::operator== (message_type const & rhs) const {
            return sender_id == rhs.sender_id && message_id == rhs.message_id &&
                   part_no == rhs.part_no && num_parts == rhs.num_parts && payload == rhs.payload;
        }

    } // end namespace brokerface
} // end namespace pstore
