//*      _             _                                            *
//*  ___| |_ __ _ _ __| |_  __   ____ _  ___ _   _ _   _ _ __ ___   *
//* / __| __/ _` | '__| __| \ \ / / _` |/ __| | | | | | | '_ ` _ \  *
//* \__ \ || (_| | |  | |_   \ V / (_| | (__| |_| | |_| | | | | | | *
//* |___/\__\__,_|_|   \__|   \_/ \__,_|\___|\__,_|\__,_|_| |_| |_| *
//*                                                                 *
//===- lib/pstore/start_vacuum.cpp ----------------------------------------===//
// Copyright (c) 2017 by Sony Interactive Entertainment, Inc. 
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

#include "pstore/start_vacuum.hpp"

// Local (public) includes
#include "pstore_broker_intf/fifo_path.hpp"
#include "pstore_broker_intf/send_message.hpp"
#include "pstore_broker_intf/writer.hpp"
#include "pstore/database.hpp"

namespace pstore {

    void start_vacuum (database const & db) {
        broker::fifo_path fifo;
        broker::writer wr (fifo);
        broker::send_message (wr, false/*error on timeout*/, "GC", db.path ().c_str ());
    }

} // namespace pstore
// eof: lib/pstore/start_vacuum.cpp
