//===- include/pstore/broker/recorder.hpp -----------------*- mode: C++ -*-===//
//*                             _            *
//*  _ __ ___  ___ ___  _ __ __| | ___ _ __  *
//* | '__/ _ \/ __/ _ \| '__/ _` |/ _ \ '__| *
//* | | |  __/ (_| (_) | | | (_| |  __/ |    *
//* |_|  \___|\___\___/|_|  \__,_|\___|_|    *
//*                                          *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file recorder.hpp
/// \brief Record and playback of messages. The recorded messages can be used
/// for later analysis and testing.

#ifndef PSTORE_BROKER_RECORDER_HPP
#define PSTORE_BROKER_RECORDER_HPP

#include <cassert>
#include <cstdio>
#include <mutex>

#include "pstore/brokerface/message_type.hpp"
#include "pstore/os/file.hpp"

namespace pstore {
    namespace broker {

        class recorder {
        public:
            explicit recorder (std::string path);
            // No copying or assignment.
            recorder (recorder const &) = delete;
            recorder (recorder &&) noexcept = delete;

            ~recorder ();

            // No copying or assignment.
            recorder & operator= (recorder const &) = delete;
            recorder & operator= (recorder &&) noexcept = delete;

            void record (brokerface::message_type const & cmd);

        private:
            std::mutex mut_;
            file::file_handle file_;
        };

        class player {
        public:
            explicit player (std::string path);
            // No copying or assignment.
            player (player const &) = delete;
            player (player &&) noexcept = delete;

            ~player ();

            // No copying or assignment.
            player & operator= (player const &) = delete;
            player & operator= (player &&) noexcept = delete;

            brokerface::message_ptr read ();

        private:
            std::mutex mut_;
            file::file_handle file_;
        };

    } // end namespace broker
} // end namespace pstore

#endif // PSTORE_BROKER_RECORDER_HPP
