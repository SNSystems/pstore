//*                           _             _                *
//* __      _____  __ _   ___| |_ __ _ _ __| |_ _   _ _ __   *
//* \ \ /\ / / __|/ _` | / __| __/ _` | '__| __| | | | '_ \  *
//*  \ V  V /\__ \ (_| | \__ \ || (_| | |  | |_| |_| | |_) | *
//*   \_/\_/ |___/\__,_| |___/\__\__,_|_|   \__|\__,_| .__/  *
//*                                                  |_|     *
//===- include/pstore_broker_intf/wsa_startup.hpp -------------------------===//
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

#ifndef PSTORE_BROKER_WSA_STARTUP_HPP
#define PSTORE_BROKER_WSA_STARTUP_HPP

namespace pstore {

#ifdef _WIN32

    class wsa_startup {
    public:
        wsa_startup () noexcept
                : started_{start ()} {}
        // no copy or assignment
        wsa_startup (wsa_startup const &) = delete;
        wsa_startup & operator= (wsa_startup const &) = delete;

        ~wsa_startup ();

        bool started () const noexcept { return started_; }

    private:
        static bool start () noexcept;
        bool started_;
    };

#endif // _WIN32

} // namespace pstore

#endif // PSTORE_BROKER_WSA_STARTUP_HPP
// eof: include/pstore_broker_intf/wsa_startup.hpp
