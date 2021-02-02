//*                     _                                             *
//*  ___  ___ _ __   __| |  _ __ ___   ___  ___ ___  __ _  __ _  ___  *
//* / __|/ _ \ '_ \ / _` | | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \ *
//* \__ \  __/ | | | (_| | | | | | | |  __/\__ \__ \ (_| | (_| |  __/ *
//* |___/\___|_| |_|\__,_| |_| |_| |_|\___||___/___/\__,_|\__, |\___| *
//*                                                       |___/       *
//===- include/pstore/brokerface/send_message.hpp -------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file send_message.hpp
/// \brief  Implements the send_message function which is the means by which the library sends
/// messages to a running pstore broker instance.

#ifndef PSTORE_BROKERFACE_SEND_MESSAGE_HPP
#define PSTORE_BROKERFACE_SEND_MESSAGE_HPP

#include <cstdint>

#include "pstore/brokerface/writer.hpp"

namespace pstore {
    /// Contains the types and functions provided by the broker interface.
    namespace brokerface {

        /// Intended for use in unit tests, returns the ID of the next message that will be
        /// dispatched by a call to send_message().
        std::uint32_t next_message_id ();

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
        void send_message (writer & wr, bool error_on_timeout, gsl::czstring verb,
                           gsl::czstring path);

    } // end namespace brokerface
} // end namespace pstore

#endif // PSTORE_BROKERFACE_SEND_MESSAGE_HPP
