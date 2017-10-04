//*                                  _   _              *
//*   __ _  ___ _ __   ___ _ __ __ _| |_(_) ___  _ __   *
//*  / _` |/ _ \ '_ \ / _ \ '__/ _` | __| |/ _ \| '_ \  *
//* | (_| |  __/ | | |  __/ | | (_| | |_| | (_) | | | | *
//*  \__, |\___|_| |_|\___|_|  \__,_|\__|_|\___/|_| |_| *
//*  |___/                                              *
//*  _ _                 _              *
//* (_) |_ ___ _ __ __ _| |_ ___  _ __  *
//* | | __/ _ \ '__/ _` | __/ _ \| '__| *
//* | | ||  __/ | | (_| | || (_) | |    *
//* |_|\__\___|_|  \__,_|\__\___/|_|    *
//*                                     *
//===- lib/pstore/generation_iterator.cpp ---------------------------------===//
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
/// \file generation_iterator.cpp

#include "pstore/generation_iterator.hpp"

#include "pstore/database.hpp"
#include "pstore/file_header.hpp"

namespace pstore {
    // ***********************
    // * generation iterator *
    // ***********************
    // (pre-increment)
    // ~~~~~~~~~~~~~~~
    generation_iterator & generation_iterator::operator++ () {
        pos_ = db_.getro<pstore::trailer> (pos_)->a.prev_generation;
        this->validate ();
        return *this;
    }

    // (post-increment)
    // ~~~~~~~~~~~~~~~~
    generation_iterator generation_iterator::operator++ (int) {
        generation_iterator prev = *this;
        ++(*this);
        return prev;
    }

    // validate
    // ~~~~~~~~
    bool generation_iterator::validate () const {
        return trailer::validate (db_, pos_);
    }


    // ************************
    // * generation container *
    // ************************
    generation_iterator generation_container::begin () {
        return {db_, db_.footer_pos ()};
    }
    generation_iterator generation_container::end () {
        return {db_, pstore::address::null ()};
    }

} // namespace pstore
// eof: lib/pstore/generation_iterator.cpp
