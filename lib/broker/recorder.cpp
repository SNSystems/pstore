//*                             _            *
//*  _ __ ___  ___ ___  _ __ __| | ___ _ __  *
//* | '__/ _ \/ __/ _ \| '__/ _` |/ _ \ '__| *
//* | | |  __/ (_| (_) | | | (_| |  __/ |    *
//* |_|  \___|\___\___/|_|  \__,_|\___|_|    *
//*                                          *
//===- lib/broker/recorder.cpp --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file recorder.cpp
/// \brief Implements record and playback of messages. The recorded messages can be used
/// for later analysis and testing.

#include "pstore/broker/recorder.hpp"

#include "pstore/broker/message_pool.hpp"
#include "pstore/support/utf.hpp"

namespace pstore {
    namespace broker {

        //*                        _          *
        //*  _ _ ___ __ ___ _ _ __| |___ _ _  *
        //* | '_/ -_) _/ _ \ '_/ _` / -_) '_| *
        //* |_| \___\__\___/_| \__,_\___|_|   *
        //*                                   *
        // (ctor)
        // ~~~~~~
        recorder::recorder (std::string path)
                : file_{std::move (path)} {
            file_.open (file::file_handle::create_mode::create_new,
                        file::file_handle::writable_mode::read_write);
        }

        // (dtor)
        // ~~~~~~
        recorder::~recorder () = default;

        // record
        // ~~~~~~
        void recorder::record (brokerface::message_type const & cmd) {
            std::unique_lock<decltype (mut_)> const lock (mut_);
            file_.write (cmd);
        }


        //*       _                    *
        //*  _ __| |__ _ _  _ ___ _ _  *
        //* | '_ \ / _` | || / -_) '_| *
        //* | .__/_\__,_|\_, \___|_|   *
        //* |_|          |__/          *
        // (ctor)
        // ~~~~~~
        player::player (std::string path)
                : file_{std::move (path)} {
            file_.open (file::file_handle::create_mode::open_existing,
                        file::file_handle::writable_mode::read_only);
        }

        // (dtor)
        // ~~~~~~
        player::~player () = default;

        // read
        // ~~~~
        brokerface::message_ptr player::read () {
            brokerface::message_ptr msg = pool.get_from_pool ();
            std::unique_lock<decltype (mut_)> const lock (mut_);
            file_.read (msg.get ());
            return msg;
        }

    } // namespace broker
} // namespace pstore
