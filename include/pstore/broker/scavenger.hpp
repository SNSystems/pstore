//*                                                 *
//*  ___  ___ __ ___   _____ _ __   __ _  ___ _ __  *
//* / __|/ __/ _` \ \ / / _ \ '_ \ / _` |/ _ \ '__| *
//* \__ \ (_| (_| |\ V /  __/ | | | (_| |  __/ |    *
//* |___/\___\__,_| \_/ \___|_| |_|\__, |\___|_|    *
//*                                |___/            *
//===- include/pstore/broker/scavenger.hpp --------------------------------===//
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
/// \file scavenger.hpp

#ifndef PSTORE_BROKER_SCAVENGER_HPP
#define PSTORE_BROKER_SCAVENGER_HPP

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace pstore {
    namespace broker {

        class command_processor;

        class scavenger {
        public:
            explicit scavenger (std::weak_ptr<command_processor> cp)
                    : cp_{cp} {}

            // no copying or deletion.
            scavenger (scavenger const &) = delete;
            scavenger & operator= (scavenger const &) = delete;

            void thread_entry ();

            void shutdown ();

        private:
            std::mutex mut_;
            std::condition_variable cv_;
            std::weak_ptr<command_processor> cp_;
        };

    } // namespace broker
} // namespace pstore
#endif // PSTORE_BROKER_SCAVENGER_HPP
