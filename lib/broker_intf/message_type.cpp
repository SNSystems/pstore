//*                                             _                     *
//*  _ __ ___   ___  ___ ___  __ _  __ _  ___  | |_ _   _ _ __   ___  *
//* | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ | __| | | | '_ \ / _ \ *
//* | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | |_| |_| | |_) |  __/ *
//* |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__|\__, | .__/ \___| *
//*                                |___/            |___/|_|          *
//===- lib/broker_intf/message_type.cpp -----------------------------------===//
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
/// \file message_type.cpp
/// \brief Contains the definition of the broker::message_type class.

#include "pstore/broker_intf/message_type.hpp"

#include <iterator>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
    inline DWORD getpid () { return ::GetCurrentProcessId (); }
#endif
} // namespace

namespace pstore {
    namespace broker {

        constexpr std::size_t message_type::payload_chars;

        std::uint32_t const message_type::process_id = static_cast<std::uint32_t> (getpid ());

        // (ctor)
        // ~~~~~~
        message_type::message_type (std::uint32_t mid, std::uint16_t pno, std::uint16_t nump,
                                    std::string const & content)
                : message_type (mid, pno, nump, std::begin (content), std::end (content)) {}

        // operator==
        // ~~~~~~~~~~
        bool message_type::operator== (message_type const & rhs) const {
            return sender_id == rhs.sender_id && message_id == rhs.message_id &&
                   part_no == rhs.part_no && num_parts == rhs.num_parts && payload == rhs.payload;
        }

    } // namespace broker
} // namespace pstore
