//*  _     _                                                 _            *
//* (_)___| |_ _ __ ___  __ _ _ __ ___    _ __ ___  __ _  __| | ___ _ __  *
//* | / __| __| '__/ _ \/ _` | '_ ` _ \  | '__/ _ \/ _` |/ _` |/ _ \ '__| *
//* | \__ \ |_| | |  __/ (_| | | | | | | | | |  __/ (_| | (_| |  __/ |    *
//* |_|___/\__|_|  \___|\__,_|_| |_| |_| |_|  \___|\__,_|\__,_|\___|_|    *
//*                                                                       *
//===- examples/serialize/istream_reader/istream_reader.cpp ---------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
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
#include <iostream>
#include <sstream>

// The serialization library
#include "pstore/serialize/archive.hpp"
#include "pstore/serialize/types.hpp"

namespace {
    class istream_reader {
    public:
        explicit istream_reader (std::istream & os)
                : os_ (os) {}

        // Reads a single instance of a standard-layout type T from the input stream and returns
        // the value extracted.
        template <typename T>
        void get (T & t) {
            static_assert (std::is_standard_layout<T>::value,
                           "istream_reader::get(T&) can only read standard-layout types");
            new (&t) T;
            os_ >> t;
        }

    private:
        std::istream & os_;
    };
} // namespace

int main () {
    std::istringstream iss{"3 73 127 179"};
    istream_reader reader{iss};

    for (auto count = 0U; count < 4U; ++count) {
        // Read a single integer from the istream_reader.
        auto value = pstore::serialize::read<int> (reader);
        std::cout << value << '\n';
    }
}
