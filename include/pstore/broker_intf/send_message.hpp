//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- include/pstore/broker_intf/send_message.hpp ------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
/// \file send_message.hpp
/// \brief  Implements the send_message function which is the means by which the library sends
/// messages to a running pstore broker instance.

#ifndef PSTORE_BROKER_INTF_SEND_MESSAGE_HPP
#define PSTORE_BROKER_INTF_SEND_MESSAGE_HPP

#include <cstdint>
#include <string>

#include "pstore/broker_intf/descriptor.hpp"
#include "pstore/support/gsl.hpp"

namespace pstore {
    namespace broker {

        /// Intended for use in unit tests, returns the ID of the next message that will be
        /// dispatched by a call to send_message().
        std::uint32_t next_message_id ();

        class writer;

        /// Sends a message consisting of a "verb" and optional "path" to the pstore broker for
        /// processing.
        ///
        /// \param wr A connection to the broker via a named pipe.
        /// \param error_on_timeout  If true, an error will be raise in the event of a
        ///    timeout. If false, this condition is silently ignored.
        /// \param verb  A null-terminated character string which contains the command that the
        ///   broker should execute.
        /// \param path  A null-terminated character string which contains the parameter for the
        ///   broker command. Pass nullptr if no parameter is required for the command.
        void send_message (writer & wr, bool error_on_timeout, ::pstore::gsl::czstring verb,
                           ::pstore::gsl::czstring path);

    } // namespace broker
} // namespace pstore
#endif // PSTORE_BROKER_INTF_SEND_MESSAGE_HPP
