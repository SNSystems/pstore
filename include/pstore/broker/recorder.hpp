//*                             _            *
//*  _ __ ___  ___ ___  _ __ __| | ___ _ __  *
//* | '__/ _ \/ __/ _ \| '__/ _` |/ _ \ '__| *
//* | | |  __/ (_| (_) | | | (_| |  __/ |    *
//* |_|  \___|\___\___/|_|  \__,_|\___|_|    *
//*                                          *
//===- include/pstore/broker/recorder.hpp ---------------------------------===//
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
/// \file recorder.hpp
/// \brief Record and playback of messages. The recorded messages can be used
/// for later analysis and testing.

#ifndef PSTORE_BROKER_RECORDER_HPP
#define PSTORE_BROKER_RECORDER_HPP

#include <cassert>
#include <cstdio>
#include <mutex>

#include "pstore/broker_intf/message_type.hpp"
#include "pstore/support/file.hpp"

namespace pstore {
    namespace broker {

        class recorder {
        public:
            explicit recorder (std::string path);
            ~recorder ();

            recorder (recorder const &) = delete;
            recorder & operator== (recorder const &) = delete;

            void record (message_type const & cmd);

        private:
            std::mutex mut_;
            file::file_handle file_;
        };

        class player {
        public:
            explicit player (std::string path);
            ~player ();

            player (player const &) = delete;
            player & operator== (player const &) = delete;

            message_ptr read ();

        private:
            std::mutex mut_;
            file::file_handle file_;
        };

    } // namespace broker
} // namespace pstore

#endif // PSTORE_BROKER_RECORDER_HPP
// eof: include/pstore/broker/recorder.hpp
