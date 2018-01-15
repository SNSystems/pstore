//*                             _            *
//*  _ __ ___  ___ ___  _ __ __| | ___ _ __  *
//* | '__/ _ \/ __/ _ \| '__/ _` |/ _ \ '__| *
//* | | |  __/ (_| (_) | | | (_| |  __/ |    *
//* |_|  \___|\___\___/|_|  \__,_|\___|_|    *
//*                                          *
//===- lib/broker/recorder.cpp --------------------------------------------===//
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
/// \file recorder.cpp
/// \brief Implements record and playback of messages. The recorded messages can be used
/// for later analysis and testing.

#include "broker/recorder.hpp"
#include "broker/message_pool.hpp"

#include "pstore_support/utf.hpp"

//*                        _          *
//*  _ _ ___ __ ___ _ _ __| |___ _ _  *
//* | '_/ -_) _/ _ \ '_/ _` / -_) '_| *
//* |_| \___\__\___/_| \__,_\___|_|   *
//*                                   *
// (ctor)
// ~~~~~~
recorder::recorder (std::string const & path) {
    file_.open (path, pstore::file::file_handle::create_mode::create_new,
                pstore::file::file_handle::writable_mode::read_write);
}

// (dtor)
// ~~~~~~
recorder::~recorder () {}

// record
// ~~~~~~
void recorder::record (pstore::broker::message_type const & cmd) {
    std::unique_lock<decltype (mut_)> lock (mut_);
    file_.write (cmd);
}


//*       _                    *
//*  _ __| |__ _ _  _ ___ _ _  *
//* | '_ \ / _` | || / -_) '_| *
//* | .__/_\__,_|\_, \___|_|   *
//* |_|          |__/          *
// (ctor)
// ~~~~~~
player::player (std::string const & path) {
    file_.open (path, pstore::file::file_handle::create_mode::open_existing,
                pstore::file::file_handle::writable_mode::read_only);
}

// (dtor)
// ~~~~~~
player::~player () {}

// read
// ~~~~
pstore::broker::message_ptr player::read () {
    pstore::broker::message_ptr msg = pool.get_from_pool ();
    std::unique_lock<decltype (mut_)> lock (mut_);
    file_.read (msg.get ());
    return msg;
}
// eof: lib/broker/recorder.cpp
