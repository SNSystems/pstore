//*                 _     _            *
//*   __ _ _ __ ___| |__ (_)_   _____  *
//*  / _` | '__/ __| '_ \| \ \ / / _ \ *
//* | (_| | | | (__| | | | |\ V /  __/ *
//*  \__,_|_|  \___|_| |_|_| \_/ \___| *
//*                                    *
//===- lib/pstore/serialize/archive.cpp -----------------------------------===//
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
/// \file serialize/archive.cpp
/// \brief Implementation of the basic archive reader and writer types.

#include "pstore/serialize/archive.hpp"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <ostream>

#include "pstore/serialize/ios_state.hpp"

namespace {
    template <typename InputIterator>
    std::ostream & hex_dump (std::ostream & os, InputIterator first, InputIterator last) {
        pstore::serialize::ios_flags_saver flags (os);
        os << std::setfill ('0') << std::hex;

        using value_type = typename std::iterator_traits<InputIterator>::value_type;
        char const * separator = "";
        std::for_each (first, last, [&](value_type v) {
            assert (v >= 0 && v <= 0xff);
            os << separator << std::setw (2) << static_cast<unsigned> (v);
            separator = " ";
        });
        return os;
    }
}


namespace pstore {
    namespace serialize {
        namespace archive {

            // *************
            // vector_writer
            // *************

            std::ostream & operator<< (std::ostream & os, vector_writer const & writer) {
                return hex_dump (os, std::begin (writer), std::end (writer));
            }


            // *************
            // buffer_writer
            // *************

            std::ostream & operator<< (std::ostream & os, buffer_writer const & writer) {
                return hex_dump (os, std::begin (writer), std::end (writer));
            }

        } // namespace archive
    }     // namespace serialize
} // namespace pstore
// eof: lib/pstore/serialize/archive.cpp
